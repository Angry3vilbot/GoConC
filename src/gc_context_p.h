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

#ifndef GOCONC_GC_CONTEXT_P_H
#define GOCONC_GC_CONTEXT_P_H
#include "timer_p.h"

typedef enum {
    GC_CONTEXT_BACKGROUND,
    GC_CONTEXT_CANCEL,
    GC_CONTEXT_TIMER,
    GC_CONTEXT_VALUE
} gc_context_type;

struct gc_context {
    gc_context_type type;
    gc_context* parent;
    gc_context* cancel_parent;
    int error;
    // Indicated by gc_context_type
    // Cancel only has these
    gc_channel* done;
    gc_context* children;
    gc_context* next_sibling;
    union {
        struct {
            gc_timer* timer;
            gc_mono_time deadline;
        } timer;

        struct {
            void* key;
            void* value;
        } value;
    };
};

void gc_context_deadline_cancel(void* arg);
#endif //GOCONC_GC_CONTEXT_P_H
