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

#ifndef GOCONC_MUTEX_P_H
#define GOCONC_MUTEX_P_H
#include "goroutine.h"
#include "waiter.h"
#include "../include/goconc/mutex.h"

typedef struct gc_mutex_impl {
    gc_goroutine* owner;
    gc_wait_queue wait_queue;
} gc_mutex_impl;

void gc_mutex_lock_impl(gc_mutex_impl* mi);
void gc_mutex_unlock_impl(gc_mutex_impl* mi);

static_assert(sizeof(gc_mutex_impl) <= GC_MUTEX_SIZE, "GC_MUTEX_SIZE is too small, make it larger");
static_assert(_Alignof(gc_mutex_impl) <= _Alignof(gc_mutex), "Alignment is wrong");

#endif //GOCONC_MUTEX_P_H
