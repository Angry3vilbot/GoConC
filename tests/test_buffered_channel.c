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


// Test that the buffered channel implementation works as intended

#include <stdio.h>
#include <stdlib.h>

#include "channel.h"
#include "scheduler.h"

void receiver(void* arg) {
    gc_channel* channel = arg;
    int received_value;
    for (int i = 0; i < 3; i++) gc_Gosched();   // let senders run first
    int status = gc_receive(channel, &received_value);
    if (status == GC_CHANNEL_OK) {
        printf("Receiver has received a value %d\n", received_value);
        fflush(stdout);
    }
}

void sender(void* arg) {
    gc_channel* channel = arg;
    int data = rand() % 100;
    printf("Sender wishes to send %d\n", data);
    fflush(stdout);
    gc_send(channel, &data);
    printf("Sender has sent %d\n", data);
    fflush(stdout);
}

int main() {
    constexpr int SENDERS = 7;
    gc_channel* channel = gc_make(sizeof(int), 3);
    // Senders first
    printf("Testing buffered channel of size 3: Senders activated first\n");
    for (int i = 0; i < SENDERS; i++) {
        go(sender, channel);
    }
    for (int i = 0; i < SENDERS; i++) {
        go(receiver, channel);
    }
    run();
    // Receivers first
    printf("Testing buffered channel of size 3: Receivers activated first\n");
    for (int i = 0; i < SENDERS; i++) {
        go(receiver, channel);
    }
    for (int i = 0; i < SENDERS; i++) {
        go(sender, channel);
    }
    run();
    gc_free(channel);
    return 0;
}
