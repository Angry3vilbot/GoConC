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

#include "mutex.h"

#include <stdio.h>
#include <stdlib.h>

#include "mutex_p.h"
#include "scheduler_p.h"

/**
* Make a Mutex Lock.
* @details Creates a new Mutex and initialises its queue.
* @warning You must call @link gc_mutex_free @endlink after you finish using the Mutex Lock
* (or at the end of the program) to free the memory allocated to it, otherwise you'll get a memory leak.
* @return The pointer to the created gc_mutex on success, <code>nullptr</code> on allocation failure
*/
gc_mutex* gc_mutex_make() {
    gc_mutex* mu = calloc(1, sizeof(*mu));
    if (mu == NULL) {
        return nullptr;
    }
    gc_wait_queue_init(&mu->wait_queue);
    return mu;
}
/**
 * Free a Mutex Lock.
 * @details Frees memory allocated to a Mutex.
 * @warning Must be called after you finish using the Mutex Lock (or at the end of the program)
 * to free the memory allocated to it, otherwise you'll get a memory leak.
 * @param mu The Mutex to free
 */
void gc_mutex_free(gc_mutex* mu) {
    if (mu == NULL) {
        // NOOP
    }
    else {
        // Check if the queue is actually empty
        if (!gc_wait_queue_isEmpty(&mu->wait_queue)) {
            fprintf(stderr, "GoConc Fatal Error: Mutex destroyed with waiting goroutines");
            abort();
        }
        free(mu);
    }
}

/// Lock a Mutex Lock
/// @details Tries to lock the Mutex. If it is already locked, the goroutine is blocked until it becomes available
/// @param mu The Mutex to lock
void gc_mutex_lock(gc_mutex* mu) {
    if (mu == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL mutex in gc_mutex_lock");
        abort();
    }
    gc_scheduler* scheduler = gc_sched_get();
    // Check if the lock is unlocked (has no owner)
    if (mu->owner == NULL) {
        // Claim the lock and return
        mu->owner = scheduler->current;
        return;
    }
    // If it isn't, enqueue and park. Once we return here we already own the lock
    // Create a waiter
    gc_waiter waiter = {
        .goroutine = scheduler->current,
        .next = nullptr,
        .data = nullptr,
        .direction = GC_WAITER_RECEIVE // Unused
    };
    // Enqueue the waiter
    gc_wait_queue_enqueue(&mu->wait_queue, &waiter);
    gc_park();
}

/// Unlock a Mutex Lock
/// @details Unlocks a locked Mutex. Aborts if the Mutex is already unlocked.
/// @note A locked Mutex can be unlocked by a different goroutine than the one that locked it.
/// @param mu The Mutex to unlock
void gc_mutex_unlock(gc_mutex* mu) {
    if (mu == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL mutex in gc_mutex_unlock");
        abort();
    }
    // Unlocking an unlocked mutex is an error, abort
    if (mu->owner == NULL) {
        fprintf(stderr, "GoConc Fatal Error: Unlock of unlocked Mutex");
        abort();
    }
    // If the queue is empty, remove the owner and return
    if (gc_wait_queue_isEmpty(&mu->wait_queue)) {
        mu->owner = nullptr;
        return;
    }
    // Otherwise, give the lock to the first waiter
    gc_waiter* waiter = gc_wait_queue_dequeue(&mu->wait_queue);
    mu->owner = waiter->goroutine;
    gc_ready(waiter->goroutine);
}

/// Try to lock a Mutex Lock and report the result
/// @note There are very few cases when one would legitimately need to use this.
/// @param mu The Mutex to try locking
/// @return <code>true</code> on success, <code>false</code> if it would be blocked
bool gc_mutex_trylock(gc_mutex* mu) {
    if (mu == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL mutex in gc_mutex_trylock");
    }
    if (mu->owner != NULL) {
        return false;
    }
    return true;
}
