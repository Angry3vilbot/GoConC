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


// Test that the default case of the select works as intended

#include <assert.h>
#include <stdio.h>

#include "channel.h"
#include "scheduler.h"
#include "select.h"

void selector(void* arg) {
    gc_channel* channel = arg;
    int recval = 0;
    int sendval = 458;

    gc_select_case cases[] = {
        {
            .channel = channel,
            .direction = GC_CASE_RECEIVE,
            .data = &recval,
            .closed = false
        },
        {
            .channel = channel,
            .direction = GC_CASE_SEND,
            .data = &sendval,
            .closed = false
        }
    };
    int index = gc_select(cases, 2, true);
    switch (index) {
        case 0:
            printf("Received from channel 1: %d\n", recval);
            break;
        case 1:
            printf("Sent to channel 1\n");
            break;
        default:
            printf("Default case\n");
    }

    assert(index == -1);
    printf("I am not blocked!\n");
}

int main() {
    gc_channel* chan = gc_make(sizeof(int), 0);

    go(selector, chan);
    run();

    gc_free(chan);

    return 0;
}
