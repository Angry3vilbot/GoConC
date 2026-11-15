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
// Test that the cancellation of a context propagates correctly.

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
}

void delayed_canceller(void* arg) {
    gc_context* context = arg;
    gc_sleep(3 * GC_SECOND);
    gc_context_cancel(context);
}

typedef struct sibling_args {
    gc_context* parent;
    gc_context* child;
    gc_context* sibling;
} sibling_args;
void sibling_canceller(void* arg) {
    sibling_args* sibling_args = arg;

    fprintf(stderr, "Cancelling child first\n");
    gc_context_cancel(sibling_args->child);
    assert(gc_context_error(sibling_args->sibling) == GC_CONTEXT_NONE);

    gc_sleep(4 * GC_SECOND);

    fprintf(stderr, "Cancelling sibling\n");
    gc_context_cancel(sibling_args->sibling);

    gc_sleep(4 * GC_SECOND);

    fprintf(stderr, "Finally, cancelling the parent so we don't get a deadlock (no main goroutine)\n");
    gc_context_cancel(sibling_args->parent);
}

int main() {
    background_context = gc_background_context();

    gc_context* cancel_test_context = gc_context_with_cancel(background_context);
    gc_context* cancel_test_child = gc_context_with_cancel(cancel_test_context);

    gc_go(context_waiter, cancel_test_context);
    gc_go(context_waiter, cancel_test_child);
    gc_go(context_waiter, cancel_test_child);

    gc_go(delayed_canceller, cancel_test_context);
    gc_run();
    gc_context_free(cancel_test_context);
    gc_context_free(cancel_test_child);

    gc_context* cancel_test_context_2 = gc_context_with_cancel(background_context);
    gc_context* cancel_test_child_2_1 = gc_context_with_cancel(cancel_test_context_2);
    gc_context* cancel_test_child_2_2 = gc_context_with_cancel(cancel_test_context_2);

    gc_go(context_waiter, cancel_test_context_2);
    gc_go(context_waiter, cancel_test_child_2_1);
    gc_go(context_waiter, cancel_test_child_2_2);

    sibling_args args = {
        .parent = cancel_test_context_2, .child = cancel_test_child_2_1, .sibling = cancel_test_child_2_2
    };
    gc_go(sibling_canceller, &args);
    gc_run();

    gc_context_free(cancel_test_child_2_1);
    gc_context_free(cancel_test_context_2);
    gc_context_free(cancel_test_child_2_2);
    return 0;
}