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

#ifndef GOCONC_CHANNEL_H
#define GOCONC_CHANNEL_H

#include <stddef.h>

typedef struct gc_channel gc_channel;

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
gc_channel* gc_make(size_t unit_size, int buffer_size);

/**
 * Free a channel.
 * @details Frees memory allocated to a channel and its buffer (if applicable).
 * @warning Must be called after you finish using the channel (or at the end of the program)
 * to free the memory allocated to the channel, otherwise you'll get a memory leak.
 * @param chan The channel to free
 */
void gc_free(gc_channel* chan);

/// Send data into the channel
/// @details Uses direct handoff when possible instead of storing the data in the buffer.
/// Unbuffered channels only use direct handoff
/// @param chan the channel to send data into
/// @param data the data to send into the channel
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed
int gc_send(gc_channel *chan, void *data);

/// Receive data from the channel
/// @details Directly receives the data into an address in memory.
/// @param chan the channel to receive data from
/// @param data the pointer to where to place the data
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed
int gc_receive(gc_channel *chan, void *data);

/// Close the channel
/// @param chan the pointer to the channel to close
/// @returns GC_CHANNEL_OK on success
int gc_close(gc_channel *chan);

/// Receive data from the channel without blocking
/// @details Directly receives the data into an address in memory.
/// @param chan the channel to receive data from
/// @param data the pointer to where to place the data
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed,
/// GC_CHANNEL_WOULD_BLOCK if the receive would block
int gc_try_receive(gc_channel *chan, void *data);

/// Send data into the channel without blocking
/// @details Uses direct handoff when possible instead of storing the data in the buffer.
/// Unbuffered channels only use direct handoff
/// @param chan the channel to send data into
/// @param data the data to send into the channel
/// @return GC_CHANNEL_OK on success, GC_CHANNEL_CLOSED if the channel was closed,
/// GC_CHANNEL_WOULD_BLOCK if the send would block
int gc_try_send(gc_channel *chan, void *data);
// Status codes
constexpr int GC_CHANNEL_OK = 0;
constexpr int GC_CHANNEL_ERROR = 1;
constexpr int GC_CHANNEL_CLOSED = 2;
constexpr int GC_CHANNEL_WOULD_BLOCK = 3;

#endif //GOCONC_CHANNEL_H
