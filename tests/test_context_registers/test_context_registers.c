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
#include <stdint.h>
#include "../../src/context.h"
// Test that the context switch keeps a context's registers intact when the other context overwrites them

static _Alignas(16) char test_stack[64 * 1024];

static gc_context main_ctx;
static gc_context test_ctx;
static int fails = 0;

int check_registers(gc_context *from, gc_context *to, uintptr_t base);

void test_func(void* p) {
    for (int base = 1; base <= 5; base++) {
        if (check_registers(&test_ctx, &main_ctx, base)) {
            fails++;
        }
    }
}

int main() {
    ctx_make(&test_ctx, test_stack, sizeof(test_stack), test_func, nullptr);

    for (int base = 1; base <= 5; base++) {
        if (check_registers(&main_ctx, &test_ctx, base)) {
            fails++;
        }
    }

    if (fails == 0) printf("Context Register test passed.\n");
    else printf("Context Register test failed, failure count = %i\n", fails);
    return fails != 0;
}