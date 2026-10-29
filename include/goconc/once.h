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

#ifndef GOCONC_ONCE_H
#define GOCONC_ONCE_H
#define GC_ONCE_SIZE 48 // The amount of bytes reserved for the Once implementation (with headroom)

typedef void (*entry_function)(void *);

/// Once
/// @details Makes sure that the given function only ever runs once
/// @warning Do NOT copy/pass by value, it will corrupt the wait queue
typedef struct gc_once {
    // Byte-Aligned reserved field for the opaque implementation
    _Alignas(16) unsigned char opaque[GC_ONCE_SIZE];
} gc_once;

/// Call the given function only once
/// @details Calls <code>func</code> if and only if @link gc_once_do @endlink is being called
/// for the first time with the given instance of <code>once</code>. All other calls made with the
/// same instance will return without calling their <code>func</code>.
/// @warning If <code>func</code> calls @link gc_once_do @endlink, you will get a deadlock.
/// @param once The pointer to the shared Once
/// @param func The function to run only once
/// @param arg The argument for the function
void gc_once_do(gc_once* once, entry_function func, void *arg);

// Initializer for readability
#define GC_ONCE_INIT {{0}}

#endif //GOCONC_ONCE_H
