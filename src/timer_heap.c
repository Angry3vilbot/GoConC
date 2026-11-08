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

#include <stdio.h>
#include <stdlib.h>

#include "../include/goconc/timer.h"

#include <time.h>

#include "timer_p.h"

bool gc_timer_heap_grow(gc_timer_heap* timer_heap) {
    size_t new_capacity = timer_heap->capacity > 0 ? timer_heap->capacity * 2 : 64;
    if (!gc_timer_heap_reserve(timer_heap, new_capacity)) {
        fprintf(stderr, "GoConC Error: Could not grow the timer heap, capacity is over the limit.\n");
        return false;
    }
    return true;
}

bool gc_timer_heap_reserve(gc_timer_heap* timer_heap, size_t new_capacity) {
    if (new_capacity > SIZE_MAX / sizeof(gc_timer*)) {
        fprintf(stderr,
            "GoConC Error: Could not reserve space in the timer heap, capacity is over the limit.\n");
        return false;
    }

    gc_timer** new_heap = realloc(timer_heap->heap, new_capacity * sizeof(new_heap));
    if (new_heap == NULL) {
        return false;
    }

    timer_heap->heap = new_heap;
    timer_heap->capacity = new_capacity;
    return true;
}

static bool compare_timers(gc_timer* t1, gc_timer* t2) {
    // Timestamp is earlier
    if (t1->when < t2->when) {
        return true;
    }
    // Timestamps are equal, seq used as tiebreaker
    if (t1->when == t2->when && t1->seq < t2->seq) {
        return true;
    }
    return false;
}

bool gc_timer_heap_push(gc_timer_heap* timer_heap, gc_timer* t) {
    // Add the timer to the end of the heap
    if (timer_heap->size < timer_heap->capacity) {
        timer_heap->heap[timer_heap->size] = t;
        t->heap_index = timer_heap->size;
        timer_heap->size++;

        t->seq = timer_heap->next_seq;
        timer_heap->next_seq++;

        t->state = GC_TIMER_HEAPED;

        gc_timer_heap_sift_up(timer_heap, t);
        return true;
    }
    // The heap has ran out of space, try and reallocate it with more
    bool status = gc_timer_heap_grow(timer_heap);
    if (!status) {
        // Couldn't grow the heap, propagate the error
        return false;
    }
    // Add the timer to the end of the heap
    timer_heap->heap[timer_heap->size] = t;
    t->heap_index = timer_heap->size;
    timer_heap->size++;
    t->state = GC_TIMER_HEAPED;
    gc_timer_heap_sift_up(timer_heap, t);
    return true;
}

gc_timer* gc_timer_heap_pop(gc_timer_heap* timer_heap) {
    // Get the timer at the root of the heap
    gc_timer* t = timer_heap->heap[0];
    // Remove the timer from the heap
    // Place the last element in the heap at the root
    timer_heap->heap[0] = timer_heap->heap[timer_heap->size - 1];
    timer_heap->heap[0]->heap_index = 0;
    timer_heap->size--;
    // Sift the heap downwards to place the element where it belongs
    gc_timer_heap_sift_down(timer_heap, timer_heap->heap[0]);
    return t;
}

gc_timer* gc_timer_heap_peek(gc_timer_heap* timer_heap) {
    // Get the timer at the root of the heap
    return timer_heap->heap[0];
}

void gc_timer_heap_remove(gc_timer_heap* timer_heap, gc_timer* t) {
    // Remove the timer from the heap
    // Place the last element in the heap at the timer's position
    timer_heap->heap[t->heap_index] = timer_heap->heap[timer_heap->size - 1];
    timer_heap->heap[t->heap_index]->heap_index = t->heap_index;
    timer_heap->size--;
    // Sift the heap downwards to place the element where it belongs
    gc_timer_heap_sift_down(timer_heap, timer_heap->heap[t->heap_index]);
}

void gc_timer_heap_sift_up(gc_timer_heap* timer_heap, gc_timer* t) {
    // While it's not the root node
    while (t->heap_index != 0) {
        size_t parent_index = (t->heap_index - 1) / 4;
        if (!compare_timers(t, timer_heap->heap[parent_index])) break;
        // Swap the timer with its parent
        timer_heap->heap[parent_index]->heap_index = t->heap_index;
        timer_heap->heap[t->heap_index] = timer_heap->heap[parent_index];
        t->heap_index = parent_index;
        timer_heap->heap[parent_index] = t;
    }
}

void gc_timer_heap_sift_down(gc_timer_heap* timer_heap, gc_timer* t) {
    size_t index = t->heap_index;
    while (4 * index + 1 < timer_heap->size) {
        size_t first_child_index = 4 * index + 1;
        size_t last_child_index = first_child_index + 3;
        // If the last child's index is out of bounds, the actual last child is at size-1
        if (last_child_index >= timer_heap->size) {
            last_child_index = timer_heap->size - 1;
        }
        // Find the smallest child node
        size_t smallest = first_child_index;
        for (size_t i = first_child_index + 1; i <= last_child_index; i++) {
            if (compare_timers(timer_heap->heap[i], timer_heap->heap[smallest])) {
                smallest = i;
            }
        }
        // Heap order above us is already correct. If the smallest child is smaller than the target,
        // That means we've finished sifting.
        if (!compare_timers(timer_heap->heap[smallest], t)) break;
        // Swap the two, without writing the target timer for now
        timer_heap->heap[index] = timer_heap->heap[smallest];
        timer_heap->heap[index]->heap_index = index;
        index = smallest;
    }
    // Write the target timer
    timer_heap->heap[index] = t;
    t->heap_index = index;
}