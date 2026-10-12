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

#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include "../src/stack.h"
// Test that the custom stack implementation works as intended
// Tests different edge cases

static gc_stack test_stack;

int main() {
    long page = sysconf(_SC_PAGESIZE);
    size_t requested_size = 64 * 1024;
    if (gc_stack_allocate(&test_stack, requested_size) != 0) return -200;
    volatile unsigned char *mem = test_stack.stack_base;
    mem[0] = 0xAA;
    mem[test_stack.stack_size - 1] = 0x55;

    if (mem[0] != 0xAA || mem[test_stack.stack_size - 1] != 0x55) return 5;
    if(page <= 0) return 4;
    if(((uintptr_t)test_stack.stack_base % page) != 0) return 3;
    if(test_stack.stack_size < requested_size) return 2;

    gc_stack_free(&test_stack);

    if (test_stack.alloc_base != NULL || test_stack.alloc_size != 0 ||
        test_stack.stack_base != NULL || test_stack.stack_size != 0) {
        return 1;
        }

    return 0;
}