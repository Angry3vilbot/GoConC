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

#include "../context.h"
// Trampoline function defined in the assembly file
extern void ctx_trampoline();
/// Initialises a new context for a goroutine by:
/// 1. Initializing the registers to fake values
/// 2. Placing the entry function pointer into the r12 register for calling
/// 3. Placing the entry function argument into the r13 register
/// 4. Storing the stack pointer inside of the newly initialised context
///
/// @param ctx: The pointer to the context of the goroutine
/// @param stack_base: The lowest address of the stack, just above the guard page
/// @param stack_size: The size (in bytes) of the usable region of the stack
/// @param entry: The pointer to the entry function
/// @param arg: The pointer to the argument of the entry function
void ctx_make(gc_context *ctx, void *stack_base, size_t stack_size, entry_function entry, void *arg) {
    // Find the top of the stack by adding the size to the base, then align it to be 16-byte aligned
    uintptr_t top = (uintptr_t) stack_base + stack_size;
    top = top & ~(uintptr_t) 15;
    // Starting from the top of the stack, reserve space for 8 slots, and cast the bottom address to a pointer
    uintptr_t *frame = (uintptr_t *)(top - 8 * sizeof(uintptr_t));
    // Initialize the registers
    frame[0] = 0;                          // r15
    frame[1] = 0;                          // r14
    frame[2] = (uintptr_t) arg;            // r13
    frame[3] = (uintptr_t) entry;          // r12
    frame[4] = 0;                          // rbp
    frame[5] = 0;                          // rbx
    frame[6] = (uintptr_t) ctx_trampoline; // trampoline function address, consumed by ret at the end of ctx_switch()
    frame[7] = 0;                          // end of the stack, should never be reached as it's a fake return address
    // Store the pointer inside of the context
    ctx->stack_pointer = frame;
}