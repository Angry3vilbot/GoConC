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

#ifndef GOCONC_TIMER_P_H
#define GOCONC_TIMER_P_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "goroutine.h"
#include "goconc/timer.h"
#include "goconc/channel.h"
#include "goconc/datetime.h"

typedef enum gc_timer_state {
    GC_TIMER_HEAPED,
    GC_TIMER_FIRED,
    GC_TIMER_STOPPED,
} gc_timer_state;

typedef union gc_timer_target {
    gc_goroutine* goroutine;
    gc_channel* channel;
} gc_timer_target;

typedef struct gc_timer gc_timer;
typedef void (*timer_function)(gc_timer* t, gc_mono_time now);

typedef struct gc_timer {
    gc_timer_state state;
    size_t heap_index;
    // A pointer to either a sleeping goroutine or a channel.
    gc_timer_target target;

    gc_mono_time when;
    // If greater than 0, it's a periodic timer that fires every period.
    int64_t period;
    timer_function function;
    uint64_t seq;
    // Whether the timer is heap-allocated or not. A sleep timer lives in the stack, other timers are allocated
    // and must be freed.
    bool heap_allocated;
} gc_timer;

typedef struct gc_timer_heap {
    // A 4-ary minheap
    gc_timer* *heap;
    size_t size;
    size_t capacity;
    // Stored in a new timer, then incremented. Used to fire equal timers in insertion order.
    uint64_t next_seq;
} gc_timer_heap;

bool gc_timer_heap_reserve(gc_timer_heap* timer_heap, size_t new_capacity);
bool gc_timer_heap_push(gc_timer_heap* timer_heap, gc_timer* t);
gc_timer* gc_timer_heap_pop(gc_timer_heap* timer_heap);
gc_timer* gc_timer_heap_peek(gc_timer_heap* timer_heap);
void gc_timer_heap_remove(gc_timer_heap* timer_heap, gc_timer* t);
void gc_timer_heap_sift_up(gc_timer_heap* timer_heap, gc_timer* t);
void gc_timer_heap_sift_down(gc_timer_heap* timer_heap, gc_timer* t);

#endif //GOCONC_TIMER_P_H
