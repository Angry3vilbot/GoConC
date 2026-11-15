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

#include <stdio.h>
#include <stdlib.h>

#include "goconc.h"
// Test that the cancellation contexts work as intended. Also inspects the background context.

gc_context* background_context;

void mixed_waiter(void* arg) {
    gc_context* context = arg;
    fprintf(stderr, "I will wait for the cancellable context to get cancelled...\n");
    fprintf(stderr, "I also have a channel which shouldn't fire in my Select...\n");
    int dummy = 0;
    gc_select_case cases[] = {
        {.channel = gc_context_done(context), .direction = GC_CASE_RECEIVE, .data = &dummy},
        {.channel = gc_make(sizeof(char), 0), .direction = GC_CASE_RECEIVE, .data = &dummy},
        {.channel = gc_make(sizeof(char), 0), .direction = GC_CASE_SEND, .data = &dummy}
    };
    switch (gc_select(cases, 3, false)) {
        case 0:
            fprintf(stderr, "Mixed waiter selected the cancelled context. Error: %d\n",
                gc_context_error(context));
            break;
        case 1:
            fprintf(stderr, "Mixed waiter received from a channel.");
            abort();
            break;
        case 2:
            fprintf(stderr, "Mixed waiter sent to a channel.");
            abort();
            break;
    }

    gc_free(cases[1].channel);
    gc_free(cases[2].channel);
}

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

int main() {
    background_context = gc_background_context();
    gc_context* todo_test = gc_todo_context(); // Gives a compiler warning

    gc_context* cancel_test_context = gc_context_with_cancel(background_context);

    gc_go(mixed_waiter, cancel_test_context);
    gc_go(context_waiter, cancel_test_context);
    gc_go(context_waiter, cancel_test_context);
    gc_go(context_waiter, cancel_test_context);

    gc_go(delayed_canceller, cancel_test_context);
    gc_run();

    gc_context_free(cancel_test_context);
    // NOOPs
    gc_context_free(todo_test);
    gc_context_free(background_context);
    return 0;
}