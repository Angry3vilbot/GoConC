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

#include "waitgroup.h"

#include <stdio.h>

#include "waitgroup_p.h"

#include <stdlib.h>

#include "scheduler_p.h"

/**
* Make a WaitGroup.
* @details Creates a new WaitGroup and initialises its queue.
* @warning You must call @link gc_wg_free @endlink after you finish using the WaitGroup
* (or at the end of the program) to free the memory allocated to it, otherwise you'll get a memory leak.
* @return The pointer to the created gc_waitgroup on success, <code>nullptr</code> on allocation failure
*/
gc_waitgroup* gc_wg_make() {
    gc_waitgroup* wg = calloc(1, sizeof(*wg));
    if (wg == NULL) {
        return nullptr;
    }
    gc_wait_queue_init(&wg->wait_queue);

    return wg;
}
/**
 * Free a WaitGroup.
 * @details Frees memory allocated to a WaitGroup.
 * @warning Must be called after you finish using the WaitGroup (or at the end of the program)
 * to free the memory allocated to it, otherwise you'll get a memory leak.
 * @param wg The WaitGroup to free
 */
void gc_wg_free(gc_waitgroup* wg) {
    if (wg == NULL) {
        // NOOP
    }
    else {
        // Check if the queue is actually empty
        if (!gc_wait_queue_isEmpty(&wg->wait_queue)) {
            fprintf(stderr, "GoConc Fatal Error: WaitGroup destroyed with waiting goroutines");
            abort();
        }
        free(wg);
    }
}

/// Add the delta to the WaitGroup's counter
/// @details Adds <code>delta</code>, which may be negative, to the WaitGroup's counter.
/// If the counter becomes zero, all goroutines blocked by @link gc_wg_wait @endlink are unblocked.
/// If the counter goes negative, the program aborts.
/// @param wg The target WaitGroup
/// @param delta The value to add to the WaitGroup's counter
void gc_wg_add(gc_waitgroup* wg, long delta) {
    if (wg == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL WaitGroup in gc_wg_add");
        abort();
    }
    // Add the delta to the count
    wg->count += delta;
    // If the count is negative, abort
    if (wg->count < 0) {
        fprintf(stderr, "GoConc Fatal Error: negative WaitGroup counter");
        abort();
    }
    // If the count is over 0, or there's no one in the queue, do nothing
    if (gc_wait_queue_isEmpty(&wg->wait_queue) || wg->count > 0) {
        return;
    }
    // The count is 0 and there are waiters, wake them all
    while (!gc_wait_queue_isEmpty(&wg->wait_queue)) {
        gc_waiter* waiter = gc_wait_queue_dequeue(&wg->wait_queue);
        gc_ready(waiter->goroutine);
    }
}

/// Decrement the WaitGroup's counter by 1
/// @details Equivalent to calling <code>gc_wg_add(wg, -1)</code>
/// @param wg The target WaitGroup
void gc_wg_done(gc_waitgroup* wg) {
    if (wg == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL WaitGroup in gc_wg_done");
        abort();
    }
    // Decrement the count
    gc_wg_add(wg, -1);
}

/// Wait until the WaitGroup's task counter is zero
/// @details Parks the goroutine until the counter reaches zero.
/// @param wg The target WaitGroup
void gc_wg_wait(gc_waitgroup* wg) {
    if (wg == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL WaitGroup in gc_wg_wait");
        abort();
    }
    // If the WaitGroup's counter is already at 0, return without blocking
    if (wg->count == 0) {
        return;
    }
    // Park the goroutine until released by gc_wg_add
    // Create a waiter
    gc_scheduler* sched = gc_sched_get();
    gc_waiter waiter = {
        .goroutine = sched->current,
        .next = nullptr,
        .data = nullptr,
        .direction = GC_WAITER_RECEIVE // Unused
    };
    // Enqueue the waiter
    gc_wait_queue_enqueue(&wg->wait_queue, &waiter);
    gc_park();
}