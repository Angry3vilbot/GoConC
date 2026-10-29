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

#ifndef GOCONC_SCHEDULER_H
#define GOCONC_SCHEDULER_H
#include <stddef.h>
typedef void (*entry_function)(void *);

constexpr size_t GC_DEFAULT_STACK_SIZE = 64 * 1024; // 64 KB

/// Start the scheduler loop.
/// @details Runs the scheduler loop until the run queue is emptied.
/// If the run queue is empty while the goroutine count is above 0, the scheduler considers it a deadlock
/// and will abort execution of the program.
void gc_run();

/// Starts a new goroutine with @link GC_DEFAULT_STACK_SIZE @endlink stack size
/// @param function the function that runs inside the goroutine
/// @param arg the argument for said function
/// @return GC_SUCCESS on success
/// GC_GOROUTINE_ALLOC_ERROR if the goroutine could not be allocated
/// GC_STACK_ALLOC_ERROR if the stack could not be allocated
int gc_go(entry_function function, void *arg);

/// Starts a new goroutine
/// @param function the function that runs inside the goroutine
/// @param arg the argument for said function
/// @param size the stack size to allocate
/// @return GC_SUCCESS on success
/// GC_GOROUTINE_ALLOC_ERROR if the goroutine could not be allocated
/// GC_STACK_ALLOC_ERROR if the stack could not be allocated
int gc_go_sized(entry_function function, void *arg, size_t size);

/// Pause the currently running goroutine and return back to the scheduler
void gc_Gosched();

// Error Codes
constexpr int GC_GOROUTINE_ALLOC_ERROR = 1;
constexpr int GC_STACK_ALLOC_ERROR = 2;
constexpr int GC_SCHEDULER_CALLED_INSIDE_GOROUTINE= 3;
constexpr int GC_SUCCESS = 0;

#endif //GOCONC_SCHEDULER_H
