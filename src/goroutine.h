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

#ifndef GOCONC_GOROUTINE_H
#define GOCONC_GOROUTINE_H
#include "context.h"
#include "stack.h"

typedef enum {
    GC_STATE_IDLE,     // Newly created goroutine awaiting to be switched to/placed in a queue
    GC_STATE_RUNNABLE, // Wants to run and is sitting in a queue waiting to be switched to
    GC_STATE_RUNNING,  // Actively executing on the thread
    GC_STATE_WAITING,  // Is paused and parked because it is blocked, for example by a channel or a mutex lock
    GC_STATE_DEAD,     // Has finished working and has been marked as terminated
} gc_state;

typedef struct gc_goroutine {
    gc_context ctx;
    gc_state status;
    gc_stack stk;
    entry_function func;
    void *arg;
    struct gc_goroutine *next; // Pointer to the next goroutine in whatever queue this goroutine is in
    struct gc_goroutine *prev; // Pointer to the previous goroutine in whatever queue this goroutine is in
    //TODO: Add a field for which case won in a select
} gc_goroutine;

#endif //GOCONC_GOROUTINE_H
