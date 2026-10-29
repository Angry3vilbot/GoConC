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

#include "../include/goconc/select.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "channel_p.h"
#include "scheduler_p.h"
#include "waiter.h"

static uint32_t state = 683756928;
/// Computes a pseudorandom number using Xorshift
uint32_t xorshift() {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

/// Get the poll order for @link gc_select @endlink
/// @details Fills an array of indices 0 - num-1 ordered randomly.
/// Used to poll the cases of a select in a fair manner.
/// Skips nullptr channels, since there's no point in polling them.
/// @param case_list The array of cases of the select
/// @param num The number of indices, equal to the number of cases in the select
/// @param poll_order The array of indices to fill
/// @return The amount of non-nullptr channels
size_t get_poll_order(gc_select_case* case_list, size_t num, size_t* poll_order) {
    size_t n = 0;
    // Loop over every case
    for (size_t i = 0; i < num; i++) {
        gc_select_case* cas = &case_list[i];
        // If we encounter a null channel, skip it
        if (cas->channel == NULL) {
            continue;
        }
        // Pick a random position from 0 to n to place the index in
        size_t pos = xorshift() % (n + 1);
        poll_order[n] = poll_order[pos];
        poll_order[pos] = i;
        n++;
    }

    return n;
}

int gc_select(gc_select_case* case_list, size_t num_cases, bool has_default) {
    // If the number of cases is over the limit, abort
    if (num_cases > GC_SELECT_MAX_CASES) {
        fprintf(stderr, "GoConC Fatal Error: Number of select cases exceeds %d\n", GC_SELECT_MAX_CASES);
        abort();
    }
    // If the number of cases is zero, and the select does NOT have a default, block forever
    if (num_cases == 0 && !has_default) {
        while (true) {
            gc_park();
        }
    }
    // Get the random poll order
    size_t poll_order[GC_SELECT_MAX_CASES];
    size_t n = get_poll_order(case_list, num_cases, poll_order);
    // If n is zero, all of the cases had null channels
    if (n == 0) {
        // If the select has a default, return -1
        if (has_default) return -1;
        // Otherwise, block the goroutine forever
        while (true) {
            gc_park();
        }
    }
    // Pass #1
    // Go through each case in the poll order
    for (size_t i = 0; i < n; i++) {
        gc_select_case* cas = &case_list[poll_order[i]];
        // Send/Receive on the channel without blocking
        int status;
        if (cas->direction == GC_CASE_RECEIVE) {
            status = gc_try_receive(cas->channel, cas->data);
        }
        else {
            status = gc_try_send(cas->channel, cas->data);
        }
        // If the operation wasn't blocked, return the index of the case
        if (status != GC_CHANNEL_WOULD_BLOCK) {
            if (status == GC_CHANNEL_CLOSED) {
                cas->closed = true;
            }
            return poll_order[i];
        }
    }
    // If all cases were blocked, and the select has default, return -1
    if (has_default) return -1;
    // Pass #2
    // Otherwise, create a waiter for each of the cases on their respective channels, then block
    gc_scheduler* scheduler = gc_sched_get();
    // Create a shared select state for the cases
    gc_select_state select_state = {
        .index = -1,
        .closed = false
    };
    gc_waiter waiters[GC_SELECT_MAX_CASES];
    for (size_t i = 0; i < n; i++) {
        gc_select_case* cas = &case_list[poll_order[i]];
        // Create a waiter
        waiters[i] = (gc_waiter){
            .goroutine = scheduler->current,
            .next = nullptr,
            .data = cas->data,
            .direction = (GC_WAITER_DIRECTION)cas->direction,
            .select_case_index = poll_order[i],
            .select_state = &select_state
        };
        // Enqueue it
        if (cas->direction == GC_CASE_RECEIVE) {
            gc_wait_queue_enqueue(&cas->channel->receivers, &waiters[i]);
        }
        else {
            gc_wait_queue_enqueue(&cas->channel->senders, &waiters[i]);
        }
    }
    // Block the goroutine
    gc_park();
    // Pass 3
    // After waking, remove all created waiters from their queues
    for (size_t i = 0; i < n; i++) {
        gc_waiter* waiter = &waiters[i];
        if (!waiter->queued) continue;

        gc_select_case* cas = &case_list[waiter->select_case_index];
        if (cas->direction == GC_CASE_RECEIVE) {
            gc_wait_queue_remove(&cas->channel->receivers, waiter);
        }
        else {
            gc_wait_queue_remove(&cas->channel->senders, waiter);
        }
    }
    // Set the closed flag on the winner if needed
    if (select_state.closed) {
        case_list[select_state.index].closed = true;
    }
    // Return the index of the winner
    return select_state.index;
}
