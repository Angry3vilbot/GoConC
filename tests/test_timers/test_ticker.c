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


// Test that the ticker works as intended

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "goconc/timer.h"
#include "goconc/scheduler.h"
#include "goconc/select.h"

void ticker(void* arg) {
    gc_timer* ticker = gc_ticker_make(5 * GC_SECOND);
    gc_channel* ticker_chan = gc_timer_get_channel(ticker);

    gc_wall_time time;
    for (int i = 0; i < 5; i++) {
        gc_receive(ticker_chan, &time);
        fprintf(stderr, "Tick Tock. Tick number: %d\n", i+1);
        fprintf(stderr, "The Unix Timestamp in seconds is: %lu\n", time/GC_SECOND);
    }
    gc_ticker_free(ticker);
}

int main() {
    gc_go(ticker, nullptr);
    gc_run();

    return 0;
}