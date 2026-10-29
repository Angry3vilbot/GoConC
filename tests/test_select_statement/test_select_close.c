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

#include <assert.h>
#include <stdio.h>

#include "channel.h"
#include "scheduler.h"
#include "select.h"
// Test that select correctly handles a channel getting closed

bool did_close = false;

void closer(void* arg) {
    gc_channel* channel = arg;
    gc_close(channel);
    printf("Closed the channel\n");
}

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
    };
    int index = gc_select(cases, 1, false);
    if (index == 0) {
        if (cases[0].closed) {
            printf("The channel got closed\n");
            did_close = true;
        }
        else {
            printf("Received from channel: %d\n", recval);
        }
    }
}

int main() {
    gc_channel* chan = gc_make(sizeof(int), 0);

    go(selector, chan);
    go(closer, chan);
    run();
    assert(did_close == true);

    gc_free(chan);

    return 0;
}
