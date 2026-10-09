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

#ifndef GOCONC_CONTEXT_H
#define GOCONC_CONTEXT_H

#include <stdint.h>
// Definition of the entry function pointer. This is the function that is called upon the goroutine's first
// time being switched in, i.e. this is the function that you pass into go(void (*fn)(void *), *void arg)
typedef void (*entry_function)(void *);

typedef struct {
    uintptr_t *stack_pointer;
} gc_context;

void ctx_switch(gc_context *from, gc_context *to);
void ctx_make(gc_context *ctx, void *stack_base, size_t stack_size, entry_function entry, void *arg);
// Assert that the stack pointer is at the top of the struct, mandatory for the assembly code to work
static_assert(offsetof(gc_context, stack_pointer) == 0);

#endif //GOCONC_CONTEXT_H
