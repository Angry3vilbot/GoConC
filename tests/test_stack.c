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
#include "../src/context.h"
#include "../src/stack.h"
// Test that the custom stack implementation (allocation and deallocation) works as intended
// Uses the test_context test as the base

static gc_stack test_stack;

static gc_context main_ctx;
static gc_context test_ctx;

void test_func(void* p) {
    int *someArg = p;
    while (true) {
        *someArg += 1;
        printf("Hello World! This is %i\n", *someArg);
        ctx_switch(&test_ctx, &main_ctx);
    }
}

int main() {
    int arg = 0;
    if (gc_stack_allocate(&test_stack, 64 * 1024) != 0) return -200;
    ctx_make(&test_ctx, test_stack.stack_base, test_stack.stack_size, test_func, &arg);
    while (arg < 5) {
        ctx_switch(&main_ctx, &test_ctx);
    }
    printf("We're back in main\n");
    gc_stack_free(&test_stack);
    if (arg != 5) return -1;
    return 0;
}