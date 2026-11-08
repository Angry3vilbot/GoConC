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


// Test that gc_after works as intended

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "goconc/timer.h"
#include "goconc/scheduler.h"
#include "goconc/select.h"

void wait(void* arg) {
    gc_channel* chan = gc_make(sizeof(int), 1);
    int dummy = 0;
    gc_wall_time time;

    gc_channel* after = gc_after(5 * GC_SECOND);
    gc_select_case cases[] = {
        {
            .channel = chan,
            .direction = GC_CASE_RECEIVE,
            .data = &dummy
        },
        {
            .channel = after,
            .direction = GC_CASE_RECEIVE,
            .data = &time
        }
    };

    int index = gc_select(cases, 2, false);
    switch (index) {
        case 0:
            fprintf(stderr, "Somehow received on the dummy channel");
            abort();
        case 1:
            fprintf(stderr, "Timed out: %lu\n", time);
            gc_free(after);
            gc_free(chan);
            break;
        default: //noop;
    }
}

void loop(void* arg) {
    for (int i = 1; i <= 4; i++) {
        gc_channel* ch = gc_after(i * GC_SECOND);
        gc_wall_time t;
        gc_receive(ch, &t);
        fprintf(stderr, "Wall time is: %lu\n", t/GC_SECOND);
        gc_free(ch);
    }
}

int main() {
    gc_go(wait, nullptr);
    gc_run();

    gc_go(loop, nullptr);
    gc_run();

    return 0;
}