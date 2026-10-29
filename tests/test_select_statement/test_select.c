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

#include "goconc/scheduler.h"
#include "goconc/select.h"
// Test that the select implementation works as intended

int results[4];
int result_index = 0;

struct channel_holder {
    gc_channel* ch1;
    gc_channel* ch2;
};

void sender(void* arg) {
    gc_channel* channel = arg;
    int sendval = 293;
    gc_send(channel, &sendval);
    printf("Sender sent %d\n", sendval);
    results[result_index++] = sendval;
}

void receiver(void* arg) {
    gc_channel* channel = arg;
    int recval = 0;
    if (gc_receive(channel, &recval) == GC_CHANNEL_OK) {
        printf("Receiver received %d\n", recval);
        results[result_index++] = recval;
    }
}

void selector(void* arg) {
    struct channel_holder* holder = arg;
    int recval = 0;
    int sendval = 458;

    gc_select_case cases[] = {
        {
            .channel = holder->ch1,
            .direction = GC_CASE_RECEIVE,
            .data = &recval,
            .closed = false
        },
        {
            .channel = holder->ch2,
            .direction = GC_CASE_SEND,
            .data = &sendval,
            .closed = false
        }
    };
    int index = gc_select(cases, 2, false);
    if (index == 1) {
        printf("Sent to channel 2\n");
        results[result_index++] = sendval;
    }
    else if (index == 0) {
        printf("Received from channel 1: %d\n", recval);
        results[result_index++] = recval;
    }
}

int main() {
    gc_channel* ch1 = gc_make(sizeof(int), 0);
    gc_channel* ch2 = gc_make(sizeof(int), 0);

    struct channel_holder holder = {
        .ch1 = ch1, .ch2 = ch2
    };
    // 1: The selector gets blocked immediately
    gc_go(selector, &holder);
    // 2: The sender sends to the first selector and exits
    gc_go(sender, ch1);
    // 3: The receiver parks because the first selector already has a winner
    gc_go(receiver, ch2);
    // 4: The selector sends to the receiver and exits
    gc_go(selector, &holder);
    // 5: The first selector wakes up and receives the data from 2
    gc_run();
    // 6: The receiver wakes up and receives the data from 4

    gc_free(ch1);
    gc_free(ch2);

    assert(result_index == 4);
    assert(results[0] == 293);
    assert(results[1] == 458);
    assert(results[2] == 293);
    assert(results[3] == 458);
    return 0;
}
