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
#include <time.h>

#include "goconc/scheduler.h"
#include "goconc/timer.h"

#include "timer_p.h"
#include "scheduler_p.h"
#include "channel_p.h"

/// Type-safe addition of a duration to a monotonic timestamp
/// Clamps the result to prevent overflow/underflow
/// @param time The monotonic timestamp
/// @param duration The duration to add to the timestamp
/// @return The resulting monotonic timestamp
gc_mono_time gc_monotonic_time_add(gc_mono_time time, gc_duration duration) {
    if (duration >= 0) {
        // Clamp
        return (time <= UINT64_MAX - duration) ? time + duration : UINT64_MAX;
    }
    // Clamp
    return (time >= -UINT64_MAX - duration) ? time + duration : -UINT64_MAX;
}

/// Type-safe addition of a duration to a wall clock timestamp
/// Clamps the result to prevent overflow/underflow
/// @param time The wall clock timestamp
/// @param duration The duration to add to the timestamp
/// @return The resulting wall clock timestamp
gc_wall_time gc_wall_time_add(gc_wall_time time, gc_duration duration) {
    if (duration >= 0) {
        // Clamp
        return (time <= UINT64_MAX - duration) ? time + duration : UINT64_MAX;
    }
    // Clamp
    return (time >= -UINT64_MAX - duration) ? time + duration : -UINT64_MAX;
}

/// Sleep timer callback function
/// Used to wake the goroutine from a timed sleep
/// @param t The sleep timer whose callback is being ran.
void gc_timer_sleep_callback(gc_timer* t, gc_mono_time now) {
    gc_goroutine* target = t->target.goroutine;
    t->state = GC_TIMER_FIRED;
    gc_ready(target);
}

/// Timer callback function
/// Used to send the current time into the channel when the timer expires/ticker ticks
/// Must never block or park
/// @param t The timer/ticker whose callback is being ran.
void gc_timer_channel_callback(gc_timer* t, gc_mono_time now) {
    // Compute the current wall clock time, to send into the channel
    gc_wall_time time = gc_wall_clock_time_now();
    // Send it into t's channel without blocking
    int status = gc_try_send(t->target.channel, &time);
    // If this is a periodic timer (a ticker), rearm it to fire again
    if (t->period > 0) {
        // If the channel is closed for some reason, stop rearming it
        if (status == GC_CHANNEL_CLOSED) {
            t->state = GC_TIMER_STOPPED;
            return;
        }

        t->when += t->period;
        if (t->when <= now) {
            t->when = gc_monotonic_time_add(now, t->period);
        }

        gc_scheduler* scheduler = gc_sched_get();
        bool res = gc_timer_heap_push(&scheduler->timer_heap, t);
        t->state = res ? GC_TIMER_HEAPED : GC_TIMER_STOPPED;
        return;
    }
    // Otherwise, it's a regular timer
    t->state = GC_TIMER_FIRED;
    // If it's owned by gc_after, free just the timer, so the user can still read from the channel
    if (t->heap_allocated) free(t);
}

gc_mono_time gc_monotonic_time_now() {
    struct timespec ts;
    int retv = clock_gettime(CLOCK_MONOTONIC, &ts);

    if (retv == -1) return -1;
    return (int64_t)ts.tv_sec * 1000000000 + (int64_t)ts.tv_nsec;
}

gc_wall_time gc_wall_clock_time_now() {
    struct timespec ts;
    int retv = clock_gettime(CLOCK_REALTIME, &ts);

    if (retv == -1) return -1;
    return (int64_t)ts.tv_sec * 1000000000 + (int64_t)ts.tv_nsec;
}

void gc_sleep(gc_duration duration) {
    // Fast path
    if (duration <= 0) gc_Gosched();
    // Create the timer on the stack
    gc_scheduler* scheduler = gc_sched_get();
    gc_timer timer;
    timer.target.goroutine = scheduler->current;
    timer.function = gc_timer_sleep_callback;
    timer.when = gc_monotonic_time_add(gc_monotonic_time_now(), duration);
    timer.heap_index = -1;
    timer.heap_allocated = false;
    // Push the timer onto the heap
    bool success = gc_timer_heap_push(&scheduler->timer_heap, &timer);
    if (!success) {
        fprintf(stderr, "GoConC Fatal Error: Sleep failed to push the timer onto the heap.\n");
        abort();
    }
    // Park the goroutine
    gc_park();
}

gc_channel* gc_after(gc_duration duration) {
    gc_timer* timer = gc_timer_make(duration);

    return timer->target.channel;
}

gc_timer* gc_timer_make(gc_duration duration) {
    // Create the timer
    gc_timer* timer = calloc(1, sizeof(*timer));
    if (timer == NULL) {
        return nullptr;
    }
    // Create the internal channel
    gc_channel* chan = gc_make(sizeof(gc_wall_time), 1);
    if (chan == NULL) {
        free(timer);
        return nullptr;
    }
    timer->target.channel = chan;
    timer->when = gc_monotonic_time_add(gc_monotonic_time_now(), duration);
    timer->function = gc_timer_channel_callback;
    timer->period = 0;
    timer->heap_index = -1;
    timer->heap_allocated = true;
    // Push it onto the heap
    gc_scheduler* scheduler = gc_sched_get();
    if (!gc_timer_heap_push(&scheduler->timer_heap, timer)) {
        gc_free(timer->target.channel);
        free(timer);
        return nullptr;
    }

    return timer;
}

void gc_timer_free(gc_timer* timer) {
    if (timer == NULL) {
        // NOOP
    }
    else {
        // Stop the timer, just in case it is still in the heap.
        gc_timer_stop(timer);
        // Free the timer's channel
        gc_free(timer->target.channel);
        free(timer);
    }
}

bool gc_timer_stop(gc_timer* timer) {
    if (timer == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: Stop called on a null timer.\n");
        abort();
    }
    // Check if the timer should be stopped
    if (timer->state == GC_TIMER_HEAPED) {
        gc_scheduler* scheduler = gc_sched_get();
        gc_timer_heap_remove(&scheduler->timer_heap, timer);
        timer->state = GC_TIMER_STOPPED;
        return true;
    }
    // Otherwise, return false
    return false;
}

int gc_timer_reset(gc_timer* timer, gc_duration duration) {
    if (timer == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: Reset called on a null timer.\n");
        abort();
    }
    // Set the timer's 'time to expire' to the new duration
    gc_mono_time old_when = timer->when;
    timer->when = gc_monotonic_time_add(gc_monotonic_time_now(), duration);

    gc_scheduler* scheduler = gc_sched_get();
    // If the timer is already heaped, sift it into position.
    if (timer->state == GC_TIMER_HEAPED) {
        // Give it a new seq number
        timer->seq = scheduler->timer_heap.next_seq;
        scheduler->timer_heap.next_seq += 1;

        gc_timer_heap_sift_up(&scheduler->timer_heap, timer);
        gc_timer_heap_sift_down(&scheduler->timer_heap, timer);

        return GC_ACTIVE_TIMER_RESET;
    }
    // Otherwise, push the timer onto the heap.
    bool result = gc_timer_heap_reserve(&scheduler->timer_heap, scheduler->timer_heap.size + 1);
    if (result == false) {
        // Failed to reserve space for the timer, restore the timer's original state
        timer->when = old_when;
        return GC_TIMER_HEAP_ALLOC_ERROR;
    }
    gc_timer_heap_push(&scheduler->timer_heap, timer);
    return GC_INACTIVE_TIMER_RESET;
}

gc_channel* gc_timer_get_channel(gc_timer* timer) {
    if (timer == nullptr) {
        return nullptr;
    }
    return timer->target.channel;
}

gc_timer* gc_ticker_make(gc_duration period) {
    // A period of 0 or less is not allowed
    if (period <= 0) {
        fprintf(stderr, "GoConC Fatal Error: Ticker's tick period must be above 0.\n");
        abort();
    }
    // Create the ticker timer
    gc_timer* ticker = calloc(1, sizeof(*ticker));
    if (ticker == NULL) {
        return nullptr;
    }
    // Create the internal channel
    gc_channel* chan = gc_make(sizeof(gc_wall_time), 1);
    if (chan == NULL) {
        free(ticker);
        return nullptr;
    }
    ticker->target.channel = chan;
    ticker->when = gc_monotonic_time_add(gc_monotonic_time_now(), period);
    ticker->function = gc_timer_channel_callback;
    ticker->period = period;
    ticker->heap_index = -1;
    // Push it onto the heap
    gc_scheduler* scheduler = gc_sched_get();
    if (!gc_timer_heap_push(&scheduler->timer_heap, ticker)) {
        gc_free(ticker->target.channel);
        free(ticker);
        return nullptr;
    }

    return ticker;
}

void gc_ticker_free(gc_timer* ticker) {
    if (ticker == NULL) {
        // NOOP
    }
    else {
        // Stop the ticker, just in case it is still in the heap.
        gc_ticker_stop(ticker);
        // Free the ticker's channel
        gc_free(ticker->target.channel);
        free(ticker);
    }
}

bool gc_ticker_stop(gc_timer* ticker) {
    if (ticker == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: Stop called on a null ticker.\n");
        abort();
    }
    // Check if the ticker should be stopped
    if (ticker->state == GC_TIMER_HEAPED) {
        gc_scheduler* scheduler = gc_sched_get();
        gc_timer_heap_remove(&scheduler->timer_heap, ticker);
        ticker->state = GC_TIMER_STOPPED;
        return true;
    }
    // Otherwise, return false
    return false;
}

int gc_ticker_reset(gc_timer* ticker, gc_duration period) {
    if (ticker == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: Reset called on a null ticker.\n");
        abort();
    }
    // Set the ticker's 'time to next tick' and tick period to new ones
    gc_mono_time old_when = ticker->when;
    gc_duration old_period = ticker->period;
    ticker->when = gc_monotonic_time_add(gc_monotonic_time_now(), period);
    ticker->period = period;

    // Give it a new seq number
    gc_scheduler* scheduler = gc_sched_get();
    uint64_t old_seq = ticker->seq;
    ticker->seq = scheduler->timer_heap.next_seq;
    scheduler->timer_heap.next_seq += 1;

    // If the ticker is already heaped, sift it into position.
    if (ticker->state == GC_TIMER_HEAPED) {
        gc_timer_heap_sift_up(&scheduler->timer_heap, ticker);
        gc_timer_heap_sift_down(&scheduler->timer_heap, ticker);

        return GC_ACTIVE_TICKER_RESET;
    }
    // Otherwise, push the ticker onto the heap.
    bool result = gc_timer_heap_reserve(&scheduler->timer_heap, scheduler->timer_heap.size + 1);
    if (result == false) {
        // Failed to reserve space for the ticker, restore the ticker's original state
        ticker->when = old_when;
        ticker->period = old_period;
        ticker->seq = old_seq;
        return GC_TIMER_HEAP_ALLOC_ERROR;
    }
    gc_timer_heap_push(&scheduler->timer_heap, ticker);
    return GC_INACTIVE_TICKER_RESET;
}