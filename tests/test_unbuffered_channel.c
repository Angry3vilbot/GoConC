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


// Test that the unbuffered channel implementation works as intended

#include <stdio.h>

#include "channel.h"
#include "scheduler.h"

void receiver(void* arg) {
    gc_channel* channel = arg;
    int received_value;
    int status = gc_receive(channel, &received_value);
    if (status == GC_CHANNEL_OK) {
        printf("Receiver has received a value %d\n", received_value);
        fflush(stdout);
    }
}

void sender(void* arg) {
    gc_channel* channel = arg;
    int data = 278;
    gc_send(channel, &data);
    printf("Sender has sent a value %d\n", data);
    fflush(stdout);
}

int main() {
    gc_channel* channel = gc_make(sizeof(int), 0);
    go(receiver, channel);
    go(sender, channel);
    run();
    // REQUIRED!!!!!!!!!!
    gc_free(channel);
    return 0;
}
