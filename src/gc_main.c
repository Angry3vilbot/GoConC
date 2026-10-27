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

#include "gc_main.h"

#include "scheduler.h"

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

/// Run a given function as the first (main) goroutine
/// @details Starts the scheduler before running the provided function using go_sized. Allows the program's
/// own entry function to use all the features that can run exclusively on a goroutine.
/// @param func The function to run
/// @param arg The function's argument
/// @return The return value of the function (the exit code)
int gc_main(int (*func)(void *), void *arg) {
    // Fill the context struct
    gc_main_context ctx = { .func = func, .arg = arg };
    // Try making a main_entry goroutine
    int result = go_sized(main_entry, &ctx, GC_MAIN_STACK_SIZE);
    if (result != GC_SUCCESS) {
        return result;
    }
    // Start the scheduler
    run();

    return ctx.result;
}
