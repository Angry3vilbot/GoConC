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


// Test that gc_sleep works as intended

#include <assert.h>
#include <stdio.h>

#include "goconc/timer.h"
#include "goconc/scheduler.h"

int test[5];
int idx = 0;

void sleeper(void* arg) {
    int* time = arg;

    fprintf(stderr, "Sleeping for %d seconds\n", *time);
    gc_sleep(*time * GC_SECOND);
    fprintf(stderr, "Woke up after sleeping for %d seconds\n", *time);
    test[idx] = *time;
    idx++;
}

int main() {
    int time1 = 1 * 2;
    int time2 = 4 * 2;
    int time3 = 2 * 2;
    int time4 = 3 * 2;
    int time5 = 5 * 2;

    gc_go(sleeper, &time1);
    gc_go(sleeper, &time2);
    gc_go(sleeper, &time3);
    gc_go(sleeper, &time4);
    gc_go(sleeper, &time5);

    gc_run();

    assert(test[0] == time1);
    assert(test[1] == time3);
    assert(test[2] == time4);
    assert(test[3] == time2);
    assert(test[4] == time5);

    return 0;
}