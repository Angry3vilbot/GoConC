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

#ifndef GOCONC_SELECT_H
#define GOCONC_SELECT_H
#define GC_SELECT_MAX_CASES 64

#include "goconc/channel.h"
#include <stdbool.h>

typedef enum {
    GC_CASE_SEND,
    GC_CASE_RECEIVE,
} GC_CASE_DIRECTION;

/// A case of a select statement
/// @details This struct represents an individual case of a select. One or more of these can be
/// placed in an array and provided to @link gc_select @endlink to be considered part of one select statement
typedef struct gc_select_case {
    /// The channel to send to/receive from
    gc_channel *channel;
    /// The direction of the operation, either @link GC_CASE_SEND @endlink or @link GC_CASE_RECEIVE @endlink.
    GC_CASE_DIRECTION direction;
    /// Pointer to the value to send or destination for the received value
    void* data;
    /// Whether the case was chosen because its channel got closed. Set to <code>false</code> by default
    bool closed;
} gc_select_case;

/// Wait on multiple channels
/// @details Lets a goroutine wait on multiple communication operations.
/// Blocks until one of its cases can run, then it executes that case.
/// Checks cases in random order for fairness.
/// @warning The maximum number of cases in a select is capped at 64
/// @param case_list The array of @link gc_select_case @endlink cases
/// @param num_cases The amount of cases in the list
/// @param has_default Whether the select operates as if it has a default case,
/// i.e. does not block if no case can run
/// @return The index of the winning case, -1 if no case is available and <code>has_default == true</code>
int gc_select(gc_select_case* case_list, size_t num_cases, bool has_default);

#endif //GOCONC_SELECT_H
