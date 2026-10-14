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

/// Starts a new goroutine
/// @param function the function that runs inside the goroutine
/// @param arg the argument for said function
/// @return GC_SUCCESS on success
/// GC_GOROUTINE_ALLOC_ERROR if the goroutine could not be allocated
/// GC_STACK_ALLOC_ERROR if the stack could not be allocated
int go(entry_function function, void *arg) {
    gc_scheduler* current = gc_sched_get();
    // Try to allocate a goroutine
    gc_goroutine* goroutine = calloc(1, sizeof(gc_goroutine));
    if (goroutine == NULL) {
        return GC_GOROUTINE_ALLOC_ERROR;
    }
    // Try to allocate a stack for the goroutine
    if (gc_stack_allocate(&goroutine->stk, GC_DEFAULT_STACK_SIZE)) {
        gc_stack_free(&goroutine->stk);
        free(goroutine);
        return GC_STACK_ALLOC_ERROR;
    }
    // Fill in the goroutine struct
    goroutine->func = function;
    goroutine->arg = arg;
    // Initialize the context of the goroutine
    ctx_make(&goroutine->ctx, goroutine->stk.stack_base, goroutine->stk.stack_size,
        goroutine->func, goroutine->arg);
    // Put the goroutine in the queue
    goroutine->status = GC_STATE_RUNNABLE;
    if (current->tail == NULL) {
        current->head = goroutine;
        current->tail = goroutine;
    }
    else {
        current->tail->next = goroutine;
        goroutine->prev = current->tail;
        current->tail = goroutine;
    }
    current->count += 1;

    return GC_SUCCESS;
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