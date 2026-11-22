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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "poll_p.h"
#include "scheduler_p.h"

/// The head of the free list from a pool of poller descriptors. Never freed.
static gc_polldesc* pd_free;

void gc_pd_unblock(gc_polldesc *pd, int direction) {
    if (direction == 0) {
        gc_goroutine* goroutine = pd->read_goroutine;
        assert(goroutine != NULL);
        pd->read_goroutine = NULL;
        pd->read_state = GC_PD_NULL;

        gc_scheduler* scheduler = gc_sched_get();
        scheduler->io_waiters--;
        assert(scheduler->io_waiters >= 0);

        gc_ready(goroutine);
        return;
    }

    gc_goroutine* goroutine = pd->write_goroutine;
    assert(goroutine != NULL);
    pd->write_goroutine = NULL;
    pd->write_state = GC_PD_NULL;

    gc_scheduler* scheduler = gc_sched_get();
    scheduler->io_waiters--;
    assert(scheduler->io_waiters >= 0);

    gc_ready(goroutine);
}

gc_polldesc* gc_pd_alloc() {
    // If the pool is not initialised yet, allocate it first
    if (pd_free == NULL) {
        // Allocate the list of poll descriptors
        gc_polldesc* pd_list = calloc(GC_PD_CHUNK_SIZE, sizeof(gc_polldesc));
        if (pd_list == NULL) {
            return nullptr;
        }
        // Link all of them together
        for (int i = 0; i < GC_PD_CHUNK_SIZE; i++) {
            // Point the current at the head
            pd_list[i].next = pd_free;
            // Make current the new head
            pd_free = &pd_list[i];
        }
    }
    // Pop a pd from the list and return it
    gc_polldesc* retv = pd_free;
    pd_free = pd_free->next;
    retv->next = nullptr;
    retv->in_pool = false;
    return retv;
}

void gc_pd_release(gc_polldesc* pd) {
    if (pd->registered) {
        fprintf(stderr, "GoConC Fatal Error - gc_pd_release: release of a registered gc_polldesc.\n");
        abort();
    }
    if (pd->write_state != GC_PD_NULL || pd->read_state != GC_PD_NULL) {
        fprintf(stderr, "GoConC Fatal Error - gc_pd_release: release of a gc_polldesc with waiters.\n");
        abort();
    }
    if (pd->in_pool) {
        fprintf(stderr, "GoConC Fatal Error - gc_pd_release: double release of a gc_polldesc.\n");
        abort();
    }

    // Preserve the sequence number
    uint32_t seq = pd->seq;
    // Zero the struct
    *pd = (gc_polldesc){0};
    // Restore the sequence number and increment it
    pd->seq = seq + 1;
    // Mark it as released so a double release aborts
    pd->in_pool = true;
    // Push it back onto the free list
    pd->next = pd_free;
    pd_free = pd;
}

int gc_poller_ready(gc_polldesc* pd, bool isReadable, bool isWritable) {
    int count = 0;

    if (isReadable) {
        switch (pd->read_state) {
            case GC_PD_NULL:
                // No one is waiting to read, set the state to GC_PD_READY for the next gc_wait_read
                pd->read_state = GC_PD_READY;
                break;
            case GC_PD_READY:
                // Do nothing, the state is already prepared
                break;
            case GC_PD_WAIT:
                // If the poll descriptor is being closed, skip readying since the close does it.
                if (pd->closing) break;
                // A goroutine is waiting to read, set the state to GC_PD_NULL and ready it
                gc_pd_unblock(pd, 0);
                count++;
                break;
        }
    }

    if (isWritable) {
        switch (pd->write_state) {
            case GC_PD_NULL:
                // No one is waiting to write, set the state to GC_PD_READY for the next gc_wait_write
                pd->write_state = GC_PD_READY;
                break;
            case GC_PD_READY:
                // Do nothing, the state is already prepared
                break;
            case GC_PD_WAIT:
                // If the poll descriptor is being closed, skip readying since the close does it.
                if (pd->closing) break;
                // A goroutine is waiting to write, set the state to GC_PD_NULL and ready it
                gc_pd_unblock(pd, 1);
                count++;
                break;
        }
    }

    return count;
}

bool gc_poll_wait_read(gc_polldesc* pd) {
    switch (pd->read_state) {
        case GC_PD_NULL:
            // Park until the operation is completed, then consume and return
            pd->read_state = GC_PD_WAIT;
            gc_scheduler* scheduler = gc_sched_get();
            pd->read_goroutine = scheduler->current;
            scheduler->io_waiters++;
            gc_park();
            // Once we're back, check if we woke up because the poll descriptor is being closed.
            // Return false if so.
            if (pd->closing) return false;
            // The operation is available now
            return true;
        case GC_PD_READY:
            // The operation is already available, consume and return
            pd->read_state = GC_PD_NULL;
            return true;
        case GC_PD_WAIT:
            // Another goroutine is somehow already waiting on the same pd, abort
            fprintf(stderr, "GoConC Runtime Error - gc_poll_wait_read: double wait.");
            abort();
        default:
            fprintf(stderr, "GoConC Runtime Error - gc_poll_wait_read: unknown read state: %d",
                pd->read_state);
            abort();
    }
}

bool gc_poll_wait_write(gc_polldesc* pd) {
    switch (pd->write_state) {
        case GC_PD_NULL:
            // Park until the operation is completed, then consume and return
            pd->write_state = GC_PD_WAIT;
            gc_scheduler* scheduler = gc_sched_get();
            pd->write_goroutine = scheduler->current;
            scheduler->io_waiters++;
            gc_park();
            // Once we're back, check if we woke up because the poll descriptor is being closed.
            // Return false if so.
            if (pd->closing) return false;
            // The operation is available now
            return true;
        case GC_PD_READY:
            // The operation is already available, consume and return
            pd->write_state = GC_PD_NULL;
            return true;
        case GC_PD_WAIT:
            // Another goroutine is somehow already waiting on the same pd, abort
            fprintf(stderr, "GoConC Runtimer Error - gc_poll_wait_write: double wait.");
            abort();
        default:
            fprintf(stderr, "GoConC Runtime Error - gc_poll_wait_read: unknown read state: %d",
                pd->read_state);
            abort();
    }
}
