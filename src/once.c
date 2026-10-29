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

#include "../include/goconc/once.h"

#include <stdio.h>
#include <stdlib.h>

#include "once_p.h"
#include "mutex_p.h"

/// Convert the gc_once into a gc_once_impl for use in functions
/// @param once The public gc_once
/// @return The private gc_once_impl
static gc_once_impl *impl(gc_once *once) {
    return (gc_once_impl *)once;
}

void gc_once_do(gc_once* once, entry_function func, void *arg) {
    gc_once_impl *once_impl = impl(once);
    if (once_impl == NULL) {
        fprintf(stderr, "GoConc Fatal Error: NULL once in gc_once_do");
        abort();
    }
    // Quick return without locking if the function already ran
    if (once_impl->done) return;
    // Lock the mutex inside gc_once
    gc_mutex_lock_impl(&once_impl->mu);
    // Check if the function has ran already
    if (!once_impl->done) {
        // Call the function if it hasn't
        func(arg);
        once_impl->done = true;
    }
    gc_mutex_unlock_impl(&once_impl->mu);
}