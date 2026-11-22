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
#include "poller.h"
#include "poll_p.h"
// Test that the descriptor pool allocation and deallocation work as intended.
// Test that poller initialization works and is idempotent.

int compare(const void* a, const void* b) {
    uintptr_t x = (uintptr_t)*(gc_polldesc * const *)a;
    uintptr_t y = (uintptr_t)*(gc_polldesc * const *)b;

    return (x > y) - (x < y);
}

void test(void* arg) {
    // Buffers for the descriptors
    gc_polldesc** pds_1 = calloc(448, sizeof(gc_polldesc*));
    gc_polldesc** pds_2 = calloc(448, sizeof(gc_polldesc*));
    // Allocate 448 descriptors
    for (int i = 0; i < 448; i++) {
        pds_1[i] = gc_pd_alloc();
    }
    // Release them all
    for (int i = 0; i < 448; i++) {
        gc_pd_release(pds_1[i]);
    }
    // Reallocate them again, they all should have seq=1
    for (int i = 0; i < 448; i++) {
        pds_2[i] = gc_pd_alloc();
        assert(pds_1[i]->seq == 1);
    }
    // Sort the two buffers and compare each pointer, they should all be the same
    qsort(pds_1, 448, sizeof(gc_polldesc*), compare);
    qsort(pds_2, 448, sizeof(gc_polldesc*), compare);
    for (int i = 0; i < 448; i++) {
        assert(pds_1[i] == pds_2[i]);
        gc_pd_release(pds_1[i]);
    }
    free(pds_1);
    free(pds_2);

    // Test idempotency of poller initialisation
    assert(gc_poller_inited() == false);
    assert(gc_poller_init() == 0);
    assert(gc_poller_inited() == true);
    assert(gc_poller_init() == 0);
    assert(gc_poller_init() == 0);
    assert(gc_poller_inited() == true);
}

int main() {
    gc_go(test, nullptr);
    gc_run();
    return 0;
}