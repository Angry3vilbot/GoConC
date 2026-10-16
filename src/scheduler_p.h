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

#ifndef GOCONC_SCHEDULER_P_H
#define GOCONC_SCHEDULER_P_H

#include "run_queue.h"
#include "goroutine.h"
#include "context.h"

typedef struct {
    gc_goroutine *current;
    gc_context context;
    // Queue
    int count;
    gc_run_queue run_queue;
    // ASan fiber parameters
    void  *asan_fake_stack;
    const void  *stack_bottom;
    size_t stack_size;
} gc_scheduler;

[[noreturn]] void goroutine_exit();
static void gc_switch(gc_context *from, gc_context *to, void **from_fake, const void *to_bottom, size_t to_size);
static void gc_goroutine_entry(void *p);

#endif //GOCONC_SCHEDULER_P_H
