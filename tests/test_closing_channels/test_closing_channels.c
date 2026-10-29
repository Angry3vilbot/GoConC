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


// Test that closing channels works as intended

#include <stdio.h>

#include "../../include/goconc/channel.h"
#include "../../include/goconc/scheduler.h"

void receiver(void* arg) {
    gc_channel* channel = arg;
    int received_value;
    int status = gc_receive(channel, &received_value);
    if (status == GC_CHANNEL_OK) {
        printf("Receiver has received a value %d\n", received_value);
        fflush(stdout);
    }
    else {
        printf("Channel was closed, value is %d\n", received_value);
        fflush(stdout);
    }
}

void sender(void* arg) {
    gc_channel* channel = arg;
    int data = 278;
    if (gc_send(channel, &data) == GC_CHANNEL_OK) {
        printf("Sender has sent a value %d\n", data);
        fflush(stdout);
    }
    else {
        printf("Sender could not send, channel was closed\n");
        fflush(stdout);
    }
}

void closer(void* arg) {
    gc_channel* channel = arg;
    gc_close(channel);
    printf("Closed the channel\n");
    fflush(stdout);
}

int main() {
    gc_channel* unbuffered = gc_make(sizeof(int), 0);
    gc_channel* buffered = gc_make(sizeof(int), 5);
    gc_channel* full_buffered = gc_make(sizeof(int), 5);

    printf("Test #1: closing an empty channel with blocked receivers.\n"
           "All receivers should wake with GC_CHANNEL_CLOSED.\n");
    for (int i = 0; i < 5; i++) {
        gc_go(receiver, unbuffered);
    }
    gc_go(closer, unbuffered);
    gc_run();

    printf("\nTest #2: closing a channel with values.\n"
           "Three receivers should wake with values, two with GC_CHANNEL_CLOSED.\n");
    int test = 123;
    for (int i = 0; i < 3; i++) {
        gc_send(buffered, &test);
    }
    for (int i = 0; i < 5; i++) {
        gc_go(receiver, buffered);
    }
    gc_go(closer, buffered);
    gc_run();

    printf("\nTest #3: closing a full channel with blocked senders.\n"
           "The senders return GC_CHANNEL_CLOSED, buffered values can still be received.\n");
    for (int i = 0; i < 10; i++) {
        gc_go(sender, full_buffered);
    }
    gc_go(closer, full_buffered);
    for (int i = 0; i < 5; i++) {
        gc_go(receiver, full_buffered);
    }
    gc_run();

    gc_free(unbuffered);
    gc_free(buffered);
    gc_free(full_buffered);
    return 0;
}