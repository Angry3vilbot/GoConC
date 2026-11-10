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

#include "goconc/datetime.h"

#include <stdio.h>
#include <time.h>

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

gc_mono_time gc_monotonic_time_add(gc_mono_time time, gc_duration duration) {
    if (duration >= 0) {
        // Clamp
        return (time <= UINT64_MAX - duration) ? time + duration : UINT64_MAX;
    }
    // Clamp
    return (time >= -UINT64_MAX - duration) ? time + duration : -UINT64_MAX;
}

gc_wall_time gc_wall_time_add(gc_wall_time time, gc_duration duration) {
    if (duration >= 0) {
        // Clamp
        return (time <= UINT64_MAX - duration) ? time + duration : UINT64_MAX;
    }
    // Clamp
    return (time >= -UINT64_MAX - duration) ? time + duration : -UINT64_MAX;
}

int64_t gc_wall_unix(gc_wall_time time) {
    int64_t sec = time / GC_SECOND;
    // If the nanosecond part is negative, subtract 1 from seconds to make them positive in gc_wall_ns.
    // See the comment in gc_wall_ns
    if (time % GC_SECOND < 0) sec -= 1;
    return sec;
}

int32_t gc_wall_ns(gc_wall_time time) {
    int32_t ns = (int32_t)(time % GC_SECOND);
    // Nanoseconds in a timestamp are always positive (they represent a forward offset from a given second)
    // If we leave the nanoseconds as negative, we will get an incorrect format string when we add the nanosecond
    // part to it. For example, -1 is "December 31, 1969 at 23:59:59.999999999", without the adjustment,
    // the format function with ns = true would instead store "January 1, 1970 at 00:00:00.-00000001"
    if (ns < 0) ns += GC_SECOND;
    return ns;
}

gc_datetime gc_wall_to_datetime(gc_wall_time time, bool utc) {
    struct tm* gmt = utc ? gmtime(&time) : localtime(&time);
    gc_datetime datetime = {
        .year = gmt->tm_year,
        .month = gmt->tm_mon,
        .day = gmt->tm_mday,
        .hours = gmt->tm_hour,
        .minutes = gmt->tm_min,
        .seconds = gmt->tm_sec,
        .nanoseconds = (int32_t)(time % GC_SECOND),
        .weekday = gmt->tm_wday,
        .utc_offset = gmt->tm_gmtoff
    };
    return datetime;
}

size_t gc_wall_format(gc_wall_time time, char* buffer, size_t size, char* format, bool utc, bool ns) {
    // gmtime/localtime only work with seconds since/before unix epoch, so we have to convert from ns
    time_t seconds = gc_wall_unix(time);
    // gmtime/localtime store the value in static storage, which is not thread-safe and can get overwritten
    // gmtime_r/localtime_r instead store it in the provided buffer, which is much better
    struct tm tm_value;
    struct tm* gmt = utc ? gmtime_r(&seconds, &tm_value) : localtime_r(&seconds, &tm_value);

    size_t bytes_written = strftime(buffer, size, format, gmt);
    if (bytes_written == 0) return 0; // Did not fit
    // Append the amount of nanoseconds to the end of the string if the user wants them
    if (ns) {
        size_t bytes_to_write = snprintf(buffer + bytes_written,
        size - bytes_written,
        ".%09d",
        gc_wall_ns(time));
        if (bytes_to_write < 0 || bytes_to_write >= size - bytes_written) {
            buffer[0] = '\0';
            return 0; // Did not fit
        }
        return bytes_written + bytes_to_write;
    }
    return bytes_written;
}
