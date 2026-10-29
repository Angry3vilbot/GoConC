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

#ifndef GOCONC_WAITGROUP_H
#define GOCONC_WAITGROUP_H

typedef void (*entry_function)(void *);
typedef struct gc_waitgroup gc_waitgroup;

/**
* Make a WaitGroup.
* @details Creates a new WaitGroup and initialises its queue.
* @warning You must call @link gc_wg_free @endlink after you finish using the WaitGroup
* (or at the end of the program) to free the memory allocated to it, otherwise you'll get a memory leak.
* @return The pointer to the created gc_waitgroup on success, <code>nullptr</code> on allocation failure
*/
gc_waitgroup* gc_wg_make();

/**
 * Free a WaitGroup.
 * @details Frees memory allocated to a WaitGroup.
 * @warning Must be called after you finish using the WaitGroup (or at the end of the program)
 * to free the memory allocated to it, otherwise you'll get a memory leak.
 * @param wg The WaitGroup to free
 */
void gc_wg_free(gc_waitgroup* wg);

/// Add the delta to the WaitGroup's counter
/// @details Adds <code>delta</code>, which may be negative, to the WaitGroup's counter.
/// If the counter becomes zero, all goroutines blocked by @link gc_wg_wait @endlink are unblocked.
/// If the counter goes negative, the program aborts.
/// @param wg The target WaitGroup
/// @param delta The value to add to the WaitGroup's counter
void gc_wg_add(gc_waitgroup* wg, long delta);

/// Decrement the WaitGroup's counter by 1
/// @details Equivalent to calling <code>gc_wg_add(wg, -1)</code>
/// @param wg The target WaitGroup
void gc_wg_done(gc_waitgroup* wg);

/// Wait until the WaitGroup's task counter is zero
/// @details Parks the goroutine until the counter reaches zero.
/// @param wg The target WaitGroup
void gc_wg_wait(gc_waitgroup* wg);

#endif //GOCONC_WAITGROUP_H
