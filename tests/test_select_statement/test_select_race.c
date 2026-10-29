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

#include "goconc.h"
// Test that only one sender/receiver can send to/receive from a select, the rest get blocked

int send_count = 0;
int recv_count = 0;
int case_count = 0;

void sender(void* arg) {
    gc_channel* channel = arg;
    int sendval = 293;
    gc_send(channel, &sendval);
    printf("Sender sent %d\n", sendval);
    send_count++;
}

void receiver(void* arg) {
    gc_channel* channel = arg;
    int recval = 0;
    if (gc_receive(channel, &recval) == GC_CHANNEL_OK) {
        printf("Receiver received %d\n", recval);
        recv_count++;
    }
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
        {
            .channel = channel,
            .direction = GC_CASE_SEND,
            .data = &sendval,
            .closed = false
        }
    };
    int index = gc_select(cases, 2, false);
    if (index == 1) {
        printf("Sent to channel 2\n");
        send_count++;
    }
    else if (index == 0) {
        printf("Received from channel 1: %d\n", recval);
        recv_count++;
    }
    case_count++;
}

int main() {
    gc_channel* chan = gc_make(sizeof(int), 0);

    gc_go(receiver, chan);
    gc_go(receiver, chan);
    gc_go(selector, chan);
    gc_go(sender, chan);
    gc_run();

    assert(case_count == 1);
    assert(send_count == recv_count);

    gc_go(sender, chan);
    gc_go(sender, chan);
    gc_go(selector, chan);
    gc_go(receiver, chan);
    gc_run();

    assert(case_count == 2);
    assert(send_count == recv_count);

    gc_free(chan);

    return 0;
}
