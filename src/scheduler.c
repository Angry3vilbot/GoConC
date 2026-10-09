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

#include "scheduler.h"

#include <assert.h>
#include <stdlib.h>

#include "scheduler_p.h"

// Static, Single-Threaded Scheduler
static gc_scheduler scheduler;

gc_scheduler* gc_sched_get() {
    return &scheduler;
}

/**
 * Exit out of the goroutine back into the scheduler
 *
 * Marks the goroutine as dead, then switches the context from the goroutine to the scheduler
 *
 * @return Must never return since the goroutine is done executing
 */
[[noreturn]] void goroutine_exit() {
    // Get the scheduler we're running on
    gc_scheduler* sched = gc_sched_get();
    // Get the current goroutine from the scheduler
    gc_goroutine* current = sched->current;
    // For testing purposes, to error out instead of silently working incorrectly
    assert(current != NULL);
    // Mark the goroutine as dead
    current->status = GC_STATE_DEAD;
    // Switch to the scheduler context
    ctx_switch(&current->ctx, &sched->context);
    // The code should never return here, if it does, then we panic
    abort();
}