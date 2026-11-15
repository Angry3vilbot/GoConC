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
// Test that the deadline contexts work as intended. Also tests cancellation's idempotence in a way (not really)

gc_context* background_context;

void context_waiter(void* arg) {
    gc_context* context = arg;
    fprintf(stderr, "I will wait for the cancellable context to get cancelled...\n");
    int dummy;
    int retv = gc_receive(gc_context_done(context), &dummy);
    fprintf(stderr,
        "Done waiting! The channel status is: %d. The context error is: %d\n",
        retv,
        gc_context_error(context));
    gc_context_cancel(context);
    assert(gc_context_error(context) == GC_CONTEXT_DEADLINE_EXCEEDED);
}

int main() {
    background_context = gc_background_context();

    gc_context* cancel_test_context = gc_context_with_timeout(background_context, 5 * GC_SECOND);

    gc_go(context_waiter, cancel_test_context);
    gc_go(context_waiter, cancel_test_context);
    gc_go(context_waiter, cancel_test_context);

    gc_run();

    gc_context_free(cancel_test_context);
    return 0;
}