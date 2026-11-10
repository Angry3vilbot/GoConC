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

#ifndef GOCONC_TIMER_H
#define GOCONC_TIMER_H

#include <stdbool.h>

#include "datetime.h"
#include "goconc/channel.h"

typedef struct gc_timer gc_timer;

/// Pauses the current goroutine for at least the given duration.
/// @note A negative/zero duration will simply yield to the scheduler without waiting.
/// @param duration The amount of time the goroutine should sleep for.
void gc_sleep(gc_duration duration);

/// Waits for the specified duration before sending the current wall clock time into the returned channel.
/// Useful for timing out operations.
/// @param duration The amount of time to wait before sending into the returned channel.
/// @return The pointer to the channel that will receive a value from this.
gc_channel* gc_after(gc_duration duration);

/**
* Make a timer.
* @details Creates a new timer and allocates memory for it. To get access to the channel that the timer sends
* to when it expires, use @link gc_timer_get_channel @endlink.
* @warning You must call @link gc_timer_free @endlink after you finish using the timer
* (or at the end of the program) to free the memory allocated to the timer, otherwise you'll get a memory leak.
* \n
* @warning Do not manually free or close the timer's channel. It will be freed by @link gc_timer_free @endlink.
* @param duration The amount of time the timer is for.
* @return The created gc_timer on success, <code>nullptr</code> on allocation failure.
*/
gc_timer* gc_timer_make(gc_duration duration);

/**
 * Free a timer.
 * @details Frees memory allocated to a timer.
 * @warning Must be called after you finish using the timer (or at the end of the program)
 * to free the memory allocated to it, otherwise you'll get a memory leak.
 * @param timer The timer to free
 */
void gc_timer_free(gc_timer* timer);

/// Stops the timer, preventing it from firing.
/// @param timer The timer to stop
/// @return <code>true</code> if the timer was stopped successfully,
/// <code>false</code> if the timer already expired or is stopped.
bool gc_timer_stop(gc_timer* timer);

/// Reset the timer to expire after the given duration.
/// @param timer The timer to reset
/// @param duration The timer's new duration
/// @return <code>GC_ACTIVE_TIMER_RESET</code> if the timer was reset while it was still active,
/// <code>GC_INACTIVE_TIMER_RESET</code> if the timer had already expired or was stopped,
/// <code>GC_TIMER_HEAP_ALLOC_ERROR</code> if the timer heap could not reserve memory for the timer
/// (in that case, the timer remains unchanged).
int gc_timer_reset(gc_timer* timer, gc_duration duration);

/// Get the timer's internal channel that it sends to when it expires.
/// @details Used when you wish to wait before executing a piece of code. When the timer expires, it will
/// send the current time into this channel.
/// @param timer The timer from which to get the channel
/// @return The pointer to the timer's channel
gc_channel* gc_timer_get_channel(gc_timer* timer);

/** Make a ticker.
* @details Creates a new ticker and allocates memory for it. The ticker sends to its channel after each tick.
* To get access to the channel that the ticker sends to when it fires, use @link gc_timer_get_channel @endlink.
* @warning You must call @link gc_ticker_free @endlink after you finish using the ticker
* (or at the end of the program) to free the memory allocated to the timer, otherwise you'll get a memory leak.
* \n
* @warning Do not manually free or close the ticker's channel. It will be freed by @link gc_ticker_free @endlink.
* @param period The duration of the tick.
* @return The created gc_timer ticker on success, <code>nullptr</code> on allocation failure.
*/
gc_timer* gc_ticker_make(gc_duration period);

/**
 * Free a ticker.
 * @details Frees memory allocated to a ticker.
 * @warning Must be called after you finish using the ticker (or at the end of the program)
 * to free the memory allocated to it, otherwise you'll get a memory leak.
 * @param ticker The ticker to free
 */
void gc_ticker_free(gc_timer* ticker);

/// Stops the ticker, preventing it from firing. No more ticks will be sent after the ticker is stopped.
/// @param ticker The ticker to stop
/// @return <code>true</code> if the ticker was stopped successfully,
/// <code>false</code> if the ticker is already stopped.
bool gc_ticker_stop(gc_timer* ticker);

/// Reset the ticker, setting its tick period to the one provided.
/// @param ticker The ticker to reset
/// @param period The new duration of the tick
/// @return <code>GC_ACTIVE_TICKER_RESET</code> if the ticker was reset while it was still active,
/// <code>GC_INACTIVE_TICKER_RESET</code> if the ticker was already stopped,
/// <code>GC_TIMER_HEAP_ALLOC_ERROR</code> if the timer heap could not reserve memory for the ticker
/// (in that case, the timer remains unchanged).
int gc_ticker_reset(gc_timer* ticker, gc_duration period);

// Status Codes
constexpr int GC_TIMER_HEAP_ALLOC_ERROR = 0;
constexpr int GC_ACTIVE_TIMER_RESET = 1;
constexpr int GC_INACTIVE_TIMER_RESET = 2;
constexpr int GC_ACTIVE_TICKER_RESET = 3;
constexpr int GC_INACTIVE_TICKER_RESET = 4;

#endif //GOCONC_TIMER_H
