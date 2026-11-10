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

#ifndef GOCONC_DATETIME_H
#define GOCONC_DATETIME_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/// Time in nanoseconds between two points.
typedef int64_t gc_duration;
/// Monotonic time in nanoseconds.
typedef int64_t gc_mono_time;
/// Wall clock time in nanoseconds since Unix epoch.
typedef int64_t gc_wall_time;

/**
 * @defgroup gc_durations Common Durations
 * @brief Common time duration constants.
 * @details Similar to Go, divide/multiply by the constant to count/convert the number of units.
 * @{
 */
#define GC_NANOSECOND ((gc_duration)1)
#define GC_MICROSECOND (GC_NANOSECOND * 1000)
#define GC_MILLISECOND (GC_MICROSECOND * 1000)
#define GC_SECOND (GC_MILLISECOND * 1000)
#define GC_MINUTE (GC_SECOND * 60)
#define GC_HOUR (GC_MINUTE * 60)
/** @} */

typedef struct gc_datetime {
    int year;
    int month;
    int day;
    int hours;
    int minutes;
    int seconds;
    int32_t nanoseconds;
    int weekday;
    long utc_offset;
} gc_datetime;

/// Get the current monotonic timestamp
/// @return Monotonic time in nanoseconds.
gc_mono_time gc_monotonic_time_now();

/// Get the current wall clock timestamp
/// @return Wall Clock time in nanoseconds since Unix epoch.
gc_wall_time gc_wall_clock_time_now();

/// Type-safe addition of a duration to a monotonic timestamp
/// Clamps the result to prevent overflow/underflow
/// @param time The monotonic timestamp
/// @param duration The duration to add to the timestamp
/// @return The resulting monotonic timestamp
gc_mono_time gc_monotonic_time_add(gc_mono_time time, gc_duration duration);

/// Type-safe addition of a duration to a wall clock timestamp
/// Clamps the result to prevent overflow/underflow
/// @param time The wall clock timestamp
/// @param duration The duration to add to the timestamp
/// @return The resulting wall clock timestamp
gc_wall_time gc_wall_time_add(gc_wall_time time, gc_duration duration);

/// Get the amount of seconds since unix epoch for a wall clock timestamp
/// @param time The wall clock timestamp
/// @return Whole seconds since unix epoch for the given timestamp
int64_t gc_wall_unix(gc_wall_time time);

/// Get the amount of nanoseconds since unix epoch for a wall clock timestamp
/// @param time The wall clock timestamp
/// @return The nanoseconds since unix epoch for the given timestamp
int32_t gc_wall_ns(gc_wall_time time);

/// Convert a wall clock timestamp into a @link gc_datetime @endlink struct
/// @param time The wall clock timestamp
/// @param utc <code>true</code> uses UTC, <code>false</code> uses the local time zone
/// @return The filled gc_datetime struct
gc_datetime gc_wall_to_datetime(gc_wall_time time, bool utc);

/// Format a wall clock timestamp.
/// @details Formats a given @link gc_wall_time @endlink according to the provided format,
/// e.g. <code>%Y-%m-%d %H:%M:%S</code>
/// @note See <code>strftime</code> documentation for more information, including on the format specifiers used.
/// @param time The wall clock timestamp to format
/// @param buffer The character buffer to write the result into
/// @param size The size of the buffer
/// @param format The format string
/// @param utc <code>true</code> uses UTC, <code>false</code> uses the local time zone
/// @param ns <code>true</code> appends the nanoseconds part of the timestamp to the string.
/// @return If the formatted string fits in the provided buffer, returns the number of bytes written.
/// Otherwise, returns 0.
size_t gc_wall_format(gc_wall_time time, char* buffer, size_t size, char* format, bool utc, bool ns);

#endif //GOCONC_DATETIME_H
