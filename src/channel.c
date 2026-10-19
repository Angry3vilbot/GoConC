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
* @return The created channel
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
    gc_channel* chan = calloc(1, sizeof(gc_channel));
    chan->unit_size = unit_size;
    //chan->buffer_size = buffer_size;
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
        // TODO: Actually do that when buffered channels are made
        // Then, destroy the channel
        free(chan);
    }
}

/// Send data into the channel
/// @details Uses direct handoff when possible instead of storing the data in the buffer.
/// Unbuffered channels only use direct handoff
/// @param chan the channel to send data into
/// @param data the data to send into the channel
/// @return GC_CHANNEL_OK on success
int gc_send(struct gc_channel *chan, void *data) {
    // Sends to a nullptr are blocked forever
    if (chan == NULL) {
        // TODO: Add a check for a non-blocking send (Select)
        gc_park();
        // We should never return here
        fprintf(stderr, "GoConc Fatal Error: Resumed a goroutine that sent on a null channel");
        abort();
    }
    // Check if there are any receivers available for a direct handoff
    if (!gc_wait_queue_isEmpty(&chan->receivers)) {
        // Directly transfer the data to the receiver
        gc_waiter* receiver = gc_wait_queue_dequeue(&chan->receivers);
        memcpy(receiver->data, data, chan->unit_size);
        // Ready the blocked receiver
        gc_ready(receiver->goroutine);
        return GC_CHANNEL_OK;
    }
    // TODO: Add branches for the buffered channel (no receiver, has room; no receiver, full)
    // If there are no receivers, enqueue and block
    // TODO: convert into buffered version
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
    // TODO: Add a check for sending on a closed channel after being unblocked
    return GC_CHANNEL_OK;
}

/// Receive data from the channel
/// @details Directly receives the data into an address in memory.
/// @param chan the channel to receive data from
/// @param data the 
/// @return GC_CHANNEL_OK on success
int gc_receive(struct gc_channel *chan, void *data) {
    // Receives from a nullptr are blocked forever
    if (chan == NULL) {
        // TODO: Add a check for a non-blocking receive (Select)
        gc_park();
        // We should never return here
        fprintf(stderr, "GoConc Fatal Error: Resumed a goroutine that received from a null channel");
        abort();
    }
    // Check if there are any senders available for a direct handoff
    if (!gc_wait_queue_isEmpty(&chan->senders)) {
        gc_waiter* sender = gc_wait_queue_dequeue(&chan->senders);
        memcpy(data, sender->data, chan->unit_size);
        // Ready the blocked sender
        gc_ready(sender->goroutine);
        return GC_CHANNEL_OK;
    }
    // TODO: Add branches for the buffered channel (no sender, has room; no sender, full)
    // If there are no senders, enqueue and block
    // TODO: convert into buffered version
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
    // TODO: Add a check for receiving on a closed channel after being unblocked
    return GC_CHANNEL_OK;
}
