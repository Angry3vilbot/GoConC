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


// Test that you cannot send after closing a channel

#include <stdio.h>

#include "../../include/goconc/channel.h"
#include "../../include/goconc/scheduler.h"

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
    gc_channel* channel = gc_make(sizeof(int), 10);
    gc_go(sender, channel);
    gc_go(closer, channel);
    gc_go(sender, channel);
    gc_run();

    gc_free(channel);
    return 0;
}
