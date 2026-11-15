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
#include <stdlib.h>

#include "goconc.h"
// Test that the value context works as intended

gc_context* background_context;

static char val_key;

typedef struct fun_args {
    gc_context* parent_context;
    gc_context* child_context;
    gc_context* value_context;
} fun_args;
void value_ctx_check(void* arg) {
    fun_args* args = arg;
    int value = 0;

    fprintf(stderr, "Checking valid key through parent\n");
    if (gc_context_value(args->parent_context, &val_key) != nullptr) {
        fprintf(stderr, "Found a value context with the given key through the parent!\n");
        abort();
    }
    fprintf(stderr, "Checking valid key through child\n");
    if (gc_context_value(args->child_context, &val_key) != nullptr) {
        value = *(int*)gc_context_value(args->child_context, &val_key);
        fprintf(stderr, "Found a value context with the given key, value = %d\n", value);
        assert(value == 612375);
        value = 0;
    }
    fprintf(stderr, "Checking valid key through value context itself\n");
    if (gc_context_value(args->value_context, &val_key) != nullptr) {
        value = *(int*)gc_context_value(args->value_context, &val_key);
        fprintf(stderr, "Found a value context with the given key, value = %d\n", value);
        assert(value == 612375);
    }

    fprintf(stderr, "Modifying value via the key.\n");
    *(int*)gc_context_value(args->value_context, &val_key) = 11188342;
}

int main() {
    background_context = gc_background_context();
    int val_value = 612375;

    gc_context* parent_context = gc_context_with_cancel(background_context);
    gc_context* value_context = gc_context_with_value(parent_context, &val_key, &val_value);
    gc_context* child_context = gc_context_with_cancel(value_context);

    fun_args args = {
        .parent_context = parent_context,
        .child_context = child_context,
        .value_context = value_context,
    };
    gc_go(value_ctx_check, &args);
    gc_run();

    gc_context_free(value_context);
    gc_context_free(parent_context);
    gc_context_free(child_context);
    assert(val_value == 11188342);
    return 0;
}