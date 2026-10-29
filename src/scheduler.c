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

#include "../include/goconc/scheduler.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "sanitizer_p.h"
#include "scheduler_p.h"

// Static, Single-Threaded Scheduler
static gc_scheduler scheduler;

gc_scheduler* gc_sched_get() {
    return &scheduler;
}

void gc_run() {
    gc_scheduler* current = gc_sched_get();
    // Check that a goroutine hasn't called run() inside itself
    assert(current->current == NULL);
    // Scheduler loop
    while (true) {
        // Check if the queue has any goroutines
        if (!gc_run_queue_isEmpty(&current->run_queue)) {
            // Dequeue a goroutine from the queue
            gc_goroutine* goroutine = gc_run_queue_dequeue(&current->run_queue);
            // Set the goroutine as running
            goroutine->status = GC_STATE_RUNNING;
            current->current = goroutine;
            // Switch the context to the goroutine
            gc_switch(&current->context, &goroutine->ctx,
                &current->asan_fake_stack, goroutine->stk.stack_base, goroutine->stk.stack_size);
            // If we return here, set current goroutine back to null
            current->current = nullptr;
            // Check if the goroutine is dead or has yielded.
            if (goroutine->status == GC_STATE_DEAD) {
                // Reap it by freeing it and decrementing the count
                gc_stack_free(&goroutine->stk);
                free(goroutine);
                current->count--;
            } else if (goroutine->status == GC_STATE_RUNNABLE) {
                // Place the goroutine back into the queue
                gc_run_queue_enqueue(&current->run_queue, goroutine);
            }
        }
        else {
            if (current->count == 0) {
                // All goroutines exited
                return;
            }
            // All goroutines are asleep, deadlock
            fprintf(stderr, "GoConC Fatal Error: all goroutines are asleep - deadlock!\n");
            // For now, abort, later on sleep until the next even and actually check for a deadlock
            abort();
        }
    }
}

int gc_go(entry_function function, void *arg) {
    return gc_go_sized(function, arg, GC_DEFAULT_STACK_SIZE);
}

int gc_go_sized(entry_function function, void *arg, size_t size) {
    gc_scheduler* current = gc_sched_get();
    // Try to allocate a goroutine
    gc_goroutine* goroutine = calloc(1, sizeof(gc_goroutine));
    if (goroutine == NULL) {
        return GC_GOROUTINE_ALLOC_ERROR;
    }
    // Try to allocate a stack for the goroutine
    if (gc_stack_allocate(&goroutine->stk, size)) {
        free(goroutine);
        return GC_STACK_ALLOC_ERROR;
    }
    // Fill in the goroutine struct
    goroutine->func = function;
    goroutine->arg = arg;
    // Initialize the context of the goroutine
    ctx_make(&goroutine->ctx, goroutine->stk.stack_base, goroutine->stk.stack_size,
        gc_goroutine_entry, goroutine);
    // Put the goroutine in the queue
    goroutine->status = GC_STATE_RUNNABLE;
    gc_run_queue_enqueue(&current->run_queue, goroutine);
    current->count += 1;

    return GC_SUCCESS;
}

void gc_Gosched() {
    gc_scheduler* current = gc_sched_get();
    gc_goroutine* goroutine = current->current;
    // Check if the scheduler actually has a running goroutine
    if (goroutine == nullptr) {
        // If not, gc_Gosched was called outside of a goroutine
        fprintf(stderr, "GoConC Fatal Error: gc_Gosched called outside of a goroutine!\n");
        abort();
    }
    // Set the goroutine's status to runnable and go back to the scheduler
    goroutine->status = GC_STATE_RUNNABLE;
    gc_switch(&goroutine->ctx, &current->context,
        &goroutine->asan_fake_stack, current->stack_bottom, current->stack_size);
}

/// Park the current goroutine
///
/// Marks the goroutine as waiting and switches back to the scheduler
void gc_park() {
    gc_scheduler* current = gc_sched_get();
    gc_goroutine* goroutine = current->current;
    // Check if the scheduler actually has a running goroutine
    if (goroutine == nullptr) {
        // If not, something's gone horribly wrong
        fprintf(stderr, "GoConC Fatal Error: Attempted to park on scheduler context!\n");
        abort();
    }
    // Set the goroutine's status to waiting and go back to the scheduler
    goroutine->status = GC_STATE_WAITING;
    gc_switch(&goroutine->ctx, &current->context,
        &goroutine->asan_fake_stack, current->stack_bottom, current->stack_size);
}

/// Ready a waiting goroutine
/// @details Marks the goroutine as runnable and enqueues it into the run queue
/// @param goroutine The goroutine to make ready
void gc_ready(gc_goroutine *goroutine) {
    gc_scheduler* current = gc_sched_get();
    assert(goroutine->status == GC_STATE_WAITING);
    // Mark the goroutine as runnable and enqueue it
    goroutine->status = GC_STATE_RUNNABLE;
    gc_run_queue_enqueue(&current->run_queue, goroutine);
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
    GC_ASAN_START(nullptr, sched->stack_bottom, sched->stack_size);
    ctx_switch(&current->ctx, &sched->context);
    // The code should never return here, if it does, then we panic
    abort();
}

/// Switch contexts with ASan fiber calls
static void gc_switch(gc_context *from, gc_context *to,
    void **from_fake, const void *to_bottom, size_t to_size) {
    GC_ASAN_START(from_fake, to_bottom, to_size);
    ctx_switch(from, to);
    GC_ASAN_END(*from_fake, nullptr, nullptr);
}

/// ASan wrapper for the first entry into the goroutine
static void gc_goroutine_entry(void *p) {
    gc_scheduler* current = gc_sched_get();
    gc_goroutine *goroutine = p;
    GC_ASAN_END(nullptr, &current->stack_bottom, &current->stack_size);
    goroutine->func(goroutine->arg);
}