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

#ifndef GOCONC_MUTEX_H
#define GOCONC_MUTEX_H
#define GC_MUTEX_SIZE 32  // The amount of bytes reserved for the mutex implementation (with headroom)
#include <stddef.h>

/// Mutex Lock
/// @warning Do NOT copy/pass by value, it will corrupt the wait queue
typedef struct gc_mutex {
    // Byte-Aligned reserved field for the opaque implementation
    _Alignas(16) unsigned char opaque[GC_MUTEX_SIZE];
} gc_mutex;

void gc_mutex_lock(gc_mutex* mu);
void gc_mutex_unlock(gc_mutex* mu);
bool gc_mutex_trylock(gc_mutex* mu);
// Initializer for readability
#define GC_MUTEX_INIT {{0}}

#endif //GOCONC_MUTEX_H
