// Copyright (c) 2026 Mykhailo Balaker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "channel.h"

#include "channel_p.h"

/**
* Make a channel.
* @details Creates a new channel and allocates memory for it and its buffer (if applicable).
* @warning You must call @link gc_free @endlink after you finish using the channel (or at the end of the program)
* to free the memory allocated to the channel, otherwise you'll get a memory leak.
* @param unit_size The size of one unit of data, e.g. <code>sizeof(int)</code>
* @param buffer_size The size of the buffer of a buffered channel.
* Use <code>0</code> for an unbuffered channel
* @return The created gc_channel on success, <code>nullptr</code> on allocation failure
*/
gc_channel* gc_make(size_t unit_size, int buffer_size) {
    // Check that the buffer/unit size is valid
    if (unit_size == 0) {
        fprintf(stderr, "GoConc Fatal Error: Channel unit size must be greater than 0");
        abort();
    }
    if (buffer_size < 0) {
        fprintf(stderr, "GoConc Fatal Error: Buffer size must be >= 0");
        abort();
    }
    // Create the channel
    gc_channel* chan = calloc(1, sizeof(*chan));
    if (chan == NULL) {
        return nullptr;
    }
    // Allocate the buffer if it's a buffered channel
    if (buffer_size > 0) {
        void* buffer = calloc(buffer_size, unit_size);
        if (buffer == NULL) {
            free(chan);
            return nullptr;
        }
        chan->buffer = buffer;
    }
    chan->unit_size = unit_size;
    chan->buffer_size = buffer_size;
    gc_wait_queue_init(&chan->senders);
    gc_wait_queue_init(&chan->receivers);
    return chan;
}

/**
 * Free a channel.
 * @details Frees memory allocated to a channel and its buffer (if applicable).
 * @warning Must be called after you finish using the channel (or at the end of the program)
 * to free the memory allocated to the channel, otherwise you'll get a memory leak.
 * @param chan The channel to free
 */
void gc_free(gc_channel* chan) {
    if (chan == NULL) {
        // NOOP
    }
    else {
        // Check if the queues are actually empty
        if (!gc_wait_queue_isEmpty(&chan->receivers) || !gc_wait_queue_isEmpty(&chan->senders)) {
            fprintf(stderr, "GoConc Fatal Error: Channel destroyed with waiting goroutines");
            abort();
        }
        // First, destroy the buffer
        free(chan->buffer);
        // Then, destroy the channel
        free(chan);
    }
}

/// Handle sending data into the channel
/// @details Uses direct handoff when possible instead of storing the data in the buffer.
/// Unbuffered channels only use direct handoff
/// @param chan the channel to send data into
/// @param data the data to send into the channel
/// @param should_block whether or not the receive is blocking
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed,
/// GC_CHANNEL_WOULD_BLOCK if the send would block with <code>should_block == false</code>
int gc_send_h(gc_channel *chan, void *data, bool should_block) {
    // Sends to a nullptr are blocked forever
    if (chan == NULL) {
        if (!should_block) return GC_CHANNEL_WOULD_BLOCK;
        gc_park();
        // We should never return here
        fprintf(stderr, "GoConc Fatal Error: Resumed a goroutine that sent on a null channel");
        abort();
    }
    // If it's sending to a closed channel, return GC_CHANNEL_CLOSED early
    if (chan->closed) {
        return GC_CHANNEL_CLOSED;
    }
    // Check if there are any receivers available for a direct handoff
    if (!gc_wait_queue_isEmpty(&chan->receivers)) {
        gc_waiter* receiver = gc_wait_queue_dequeue_claimed(&chan->receivers);
        if (receiver != nullptr) {
            // Directly transfer the data to the receiver
            memcpy(receiver->data, data, chan->unit_size);
            // Ready the blocked receiver
            gc_ready(receiver->goroutine);
            return GC_CHANNEL_OK;
        }
    }
    // If there is no receiver, but there is room in the buffer, copy the data into the buffer
    if (chan->count < chan->buffer_size) {
        size_t buffer_slot = (chan->buffer_head_index + chan->count) % chan->buffer_size;
        memcpy(chan->buffer + buffer_slot * chan->unit_size, data, chan->unit_size);
        chan->count++;
        // Return without blocking
        return GC_CHANNEL_OK;
    }
    // If there are no receivers, and there's no room in the buffer/channel is unbuffered, enqueue and block
    if (!should_block) return GC_CHANNEL_WOULD_BLOCK;
    // Create a waiter
    gc_scheduler* sched = gc_sched_get();
    gc_waiter waiter = {
        .goroutine = sched->current,
        .next = nullptr,
        .data = data,
        .direction = GC_WAITER_SEND
    };
    // Enqueue the waiter
    gc_wait_queue_enqueue(&chan->senders, &waiter);
    gc_park();
    return waiter.closed ? GC_CHANNEL_CLOSED : GC_CHANNEL_OK;
}
/// Send data into the channel
/// @details Uses direct handoff when possible instead of storing the data in the buffer.
/// Unbuffered channels only use direct handoff
/// @param chan the channel to send data into
/// @param data the data to send into the channel
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed
int gc_send(gc_channel *chan, void *data) {
    return gc_send_h(chan, data, true);
}
/// Send data into the channel without blocking
/// @details Uses direct handoff when possible instead of storing the data in the buffer.
/// Unbuffered channels only use direct handoff
/// @param chan the channel to send data into
/// @param data the data to send into the channel
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed,
/// GC_CHANNEL_WOULD_BLOCK if the send would block
int gc_try_send(gc_channel *chan, void *data) {
    return gc_send_h(chan, data, false);
}


/// Handle receiving data from the channel
/// @details Directly receives the data into an address in memory.
/// @param chan the channel to receive data from
/// @param data the pointer to where to place the data
/// @param should_block whether or not the receive is blocking
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed,
/// GC_CHANNEL_WOULD_BLOCK if the receive would block with <code>should_block == false</code>
int gc_receive_h(gc_channel *chan, void *data, bool should_block) {
    // Receives from a nullptr are blocked forever
    if (chan == NULL) {
        if (!should_block) return GC_CHANNEL_WOULD_BLOCK;
        gc_park();
        // We should never return here
        fprintf(stderr, "GoConc Fatal Error: Resumed a goroutine that received from a null channel");
        abort();
    }
    // If the buffer has data in it, copy the data from the buffer and process a waiting sender (if there is one)
    if (chan->buffer_size != 0 && chan->count > 0) {
        memcpy(data, chan->buffer + chan->buffer_head_index * chan->unit_size, chan->unit_size);
        chan->buffer_head_index = (chan->buffer_head_index + 1) % chan->buffer_size;
        chan->count--;
        // Check if there is a sender waiting to put data into the buffer
        if (!gc_wait_queue_isEmpty(&chan->senders)) {
            // Copy the data into the queue
            gc_waiter* sender = gc_wait_queue_dequeue(&chan->senders);
            size_t buffer_slot = (chan->buffer_head_index + chan->count) % chan->buffer_size;
            memcpy(chan->buffer + buffer_slot * chan->unit_size, sender->data, chan->unit_size);
            chan->count++;
            // Ready the sender
            gc_ready(sender->goroutine);
        }
        // Return without blocking
        return GC_CHANNEL_OK;
    }
    // Check if there are any senders available for a direct handoff
    if (!gc_wait_queue_isEmpty(&chan->senders)) {
        gc_waiter* sender = gc_wait_queue_dequeue_claimed(&chan->senders);
        if (sender != nullptr) {
            memcpy(data, sender->data, chan->unit_size);
            // Ready the blocked sender
            gc_ready(sender->goroutine);
            return GC_CHANNEL_OK;
        }
    }
    // If the channel is closed, zero the data if needed and return
    if (chan->closed) {
        if (data != nullptr) {
            memset(data, 0, chan->unit_size);
        }
        return GC_CHANNEL_CLOSED;
    }
    // If there are no senders, and there's no room in the buffer/channel is unbuffered, enqueue and block
    if (!should_block) return GC_CHANNEL_WOULD_BLOCK;
    // Create a waiter
    gc_scheduler* sched = gc_sched_get();
    gc_waiter waiter = {
        .goroutine = sched->current,
        .next = nullptr,
        .data = data,
        .direction = GC_WAITER_RECEIVE
    };
    // Enqueue the waiter
    gc_wait_queue_enqueue(&chan->receivers, &waiter);
    gc_park();
    return waiter.closed ? GC_CHANNEL_CLOSED : GC_CHANNEL_OK;
}
/// Receive data from the channel
/// @details Directly receives the data into an address in memory.
/// @param chan the channel to receive data from
/// @param data the pointer to where to place the data
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed
int gc_receive(gc_channel *chan, void *data) {
    return gc_receive_h(chan, data, true);
}
/// Receive data from the channel without blocking
/// @details Directly receives the data into an address in memory.
/// @param chan the channel to receive data from
/// @param data the pointer to where to place the data
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed,
/// GC_CHANNEL_WOULD_BLOCK if the receive would block
int gc_try_receive(gc_channel *chan, void *data) {
    return gc_receive_h(chan, data, false);
}

/// Close the channel
/// @param chan the pointer to the channel to close
/// @returns GC_CHANNEL_OK on success
int gc_close(gc_channel *chan) {
    // Closing a null channel is an error
    if (chan == NULL) {
        fprintf(stderr, "GoConc Fatal Error: Closed a null channel");
        abort();
    }
    // So is closing an already closed channel
    if (chan->closed) {
        fprintf(stderr, "GoConc Fatal Error: Closed an already closed channel");
        abort();
    }
    // Close the channel
    chan->closed = true;
    // Wake all blocked receivers
    while (!gc_wait_queue_isEmpty(&chan->receivers)) {
        gc_waiter* receiver = gc_wait_queue_dequeue(&chan->receivers);
        // Check if the waiter is stale
        bool is_stale = gc_waiter_is_stale(receiver);
        // Skip it if it is
        if (is_stale) {
            continue;
        }
        // Zero the data buffer
        memset(receiver->data, 0, chan->unit_size);
        receiver->closed = true;
        if (receiver->select_state != NULL) {
            receiver->select_state->closed = true;
        }
        gc_ready(receiver->goroutine);
    }
    // Wake all blocked senders, they'll return GC_CHANNEL_CLOSED upon waking
    while (!gc_wait_queue_isEmpty(&chan->senders)) {
        gc_waiter* sender = gc_wait_queue_dequeue(&chan->senders);
        // Check if the waiter is stale
        bool is_stale = gc_waiter_is_stale(sender);
        // Skip it if it is
        if (is_stale) {
            continue;
        }
        sender->closed = true;
        if (sender->select_state != NULL) {
            sender->select_state->closed = true;
        }
        gc_ready(sender->goroutine);
    }

    return GC_CHANNEL_OK;
}