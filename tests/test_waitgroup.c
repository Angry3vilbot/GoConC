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

// Test that the WaitGroup implementation works as intended

#include <assert.h>
#include <stdio.h>

#include "../include/goconc/scheduler.h"
#include "../include/goconc/waitgroup.h"

struct test_struct {
    int check_value;
    gc_waitgroup* wg;
};

void worker(void* args) {
    struct test_struct* test = args;
    gc_waitgroup* wg = test->wg;

    printf("Worker working...\n");
    test->check_value += 2;
    gc_wg_done(wg);
    printf("Worker finished the work.\n");
}

void waiter(void* args) {
    struct test_struct* test = args;
    gc_waitgroup* wg = test->wg;

    printf("Waiting...\n");
    gc_wg_wait(wg);
    test->check_value *= 2;
    test->check_value++;
    printf("Done Waiting!\n");
}

int main() {
    gc_waitgroup* wg = gc_wg_make();
    struct test_struct test;
    test.check_value = 0;
    test.wg = wg;
    // Test #1: Waiting on a zero count does not block
    gc_go(waiter, &test);
    gc_run();
    assert(test.check_value == 1);
    printf("\n");
    // Test #2: Waiter completes after all workers are done
    gc_wg_add(wg, 10);
    gc_go(waiter, &test);
    for (int i = 0; i < 10; i++) {
        gc_go(worker, &test);
    }
    gc_run();
    assert(test.check_value == 43);
    printf("\n");
    // Test #3: Multiple waiters on one WaitGroup all wake up
    gc_wg_add(wg, 1);
    gc_go(waiter, &test);
    gc_go(waiter, &test);
    gc_go(worker, &test);
    gc_run();
    assert(test.check_value == 183);

    gc_wg_free(wg);
    return 0;
}
