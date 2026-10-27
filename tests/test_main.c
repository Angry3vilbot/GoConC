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
#include "gc_main.h"
#include "scheduler.h"
// Test that the gc_main implementation works as intended

void sender(void* arg) {
    gc_channel* chan = arg;
    int data = 27;
    gc_send(chan, &data);
}

int actual_main(void* arg) {
    int* check = arg;
    printf("Do Stuff\n");
    *check = 100;

    gc_channel* chan = gc_make(sizeof(int), 0);
    go(sender, chan);
    int received = 0;
    if (gc_receive(chan, &received) == GC_CHANNEL_OK) {
        printf("Data received successfully: %d\n", received);
        assert(received == 27);
    }
    else {
        printf("Failed to receive from the channel\n");
    }

    gc_free(chan);
    return 123;
}

int main() {
    int arg_check = 67;
    int retv = gc_main(actual_main, &arg_check);

    assert(retv == 123);
    assert(arg_check == 100);
    return 0;
}
