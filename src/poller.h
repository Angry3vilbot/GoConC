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

#ifndef GOCONC_POLLER_H
#define GOCONC_POLLER_H

#define GC_POLLER_EVENT_BUFFER_SIZE 128

#include <stdbool.h>

#include "poll_p.h"
#include "goconc/datetime.h"

/// Initialize the poller. Can be called multiple times.
/// @return 0 on success, -1 on failure
int gc_poller_init();

/// Check if the poller's initialised.
/// @return <code>true</code> if it is already initialised, <code>false</code> otherwise.
bool gc_poller_inited();

/// Add the poll descriptor to the running poller.
/// @param pd The poll descriptor to add
/// @return 0 on success, -1 on failure
int gc_poller_add(gc_polldesc* pd);

/// Remove a poll descriptor from the running poller.
/// @param pd The poll descriptor to remove
/// @return 0 on success, -1 on failure
int gc_poller_del(gc_polldesc* pd);

/// Check for ready events on the running poller.
/// @param delay The amount of time to block for
/// @return The number of goroutines woken up
int gc_poller_poll(gc_duration delay);

#endif //GOCONC_POLLER_H
