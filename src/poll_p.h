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

#ifndef GOCONC_POLL_P_H
#define GOCONC_POLL_P_H
#include <stdint.h>
#include <stdbool.h>

#include "goroutine.h"

static const size_t GC_PD_CHUNK_SIZE = 128;

typedef enum {
    GC_PD_NULL,
    GC_PD_READY,
    GC_PD_WAIT
} gc_poll_state;

/// Netpoller descriptor
typedef struct gc_polldesc {
    int file_descriptor;
    uint32_t seq;

    gc_poll_state read_state;
    gc_poll_state write_state;
    gc_goroutine* read_goroutine;
    gc_goroutine* write_goroutine;
    uint32_t read_seq;
    uint32_t write_seq;

    bool closing;
    bool registered;
    bool in_pool;

    struct gc_polldesc *next;
} gc_polldesc;

/// Unblock a goroutine waiting in a gc_polldesc
/// @param pd The poll descriptor
/// @param direction 0 for Read, 1 for Write
void gc_pd_unblock(gc_polldesc *pd, int direction);

/// Allocates the pool of gc_polldesc if not done already, then pops a free pd from the list.
/// @return A pointer to a pd from the pool, <code>nullptr</code> on pool allocation failure
gc_polldesc* gc_pd_alloc();

/// Zeroes a gc_polldesc (excluding the sequence number) and pushes it back onto the free list
/// @param pd The gc_polldesc to release
void gc_pd_release(gc_polldesc* pd);

/// Unblocks goroutines that waited using @link gc_poll_wait_read @endlink and/or @link gc_poll_wait_write @endlink
/// for a given poll descriptor depending on the provided booleans received from an event.
/// @details Moves pd.read_state and/or pd.write_state to GC_PD_READY if no goroutines are waiting on the event.
/// Otherwise, moves the state(s) to GC_PD_NULL and readies the respective goroutine
/// @param pd The poll descriptor to apply the event to
/// @param isReadable Is the read operation available
/// @param isWritable Is the write operation available
/// @return The number of goroutines readied
int gc_poller_ready(gc_polldesc* pd, bool isReadable, bool isWritable);

/// Wait until a Read operation is available. Concurrent gc_poll_wait_read calls are forbidden since only one
/// goroutine can be held by a gc_polldesc for each mode.
/// @param pd The poll descriptor to wait on
/// @return <code>true</code> if the read operation is ready, <code>false</code> if timed out or closed
bool gc_poll_wait_read(gc_polldesc* pd);

/// Wait until a Write operation is available. Concurrent gc_poll_wait_write calls are forbidden since only one
/// goroutine can be held by a gc_polldesc for each mode.
/// @param pd The poll descriptor to wait on
/// @return <code>true</code> if the write operation is ready, <code>false</code> if timed out or closed
bool gc_poll_wait_write(gc_polldesc* pd);

#endif //GOCONC_POLL_P_H
