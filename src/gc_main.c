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

#include "../include/goconc/gc_main.h"

#include "../include/goconc/scheduler.h"

static constexpr size_t GC_MAIN_STACK_SIZE = 1024 * 1024; // 1 MB

typedef struct gc_main_context  {
    int (*func)(void *);
    void* arg;
    int result;
} gc_main_context;

static void main_entry(void *p) {
    gc_main_context* ctx = p;
    int retval = ctx->func(ctx->arg);
    ctx->result = retval;
}

int gc_main(int (*func)(void *), void *arg) {
    // Fill the context struct
    gc_main_context ctx = { .func = func, .arg = arg };
    // Try making a main_entry goroutine
    int result = gc_go_sized(main_entry, &ctx, GC_MAIN_STACK_SIZE);
    if (result != GC_SUCCESS) {
        return result;
    }
    // Start the scheduler
    gc_run();

    return ctx.result;
}
