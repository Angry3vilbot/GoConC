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
#include <goconc.h>
#include <stdio.h>
#include <string.h>
// Test that the datetime functions work as intended

int main() {
    gc_wall_time timestamps[7] = {
        0, // 1970-01-01 00:00:00
        1794097992674863265, // 2026-11-08 00:33:12.674
        1709210096000000000, // 2024-02-29 12:34:56
        4107542400000000000, // 2100-03-01 00:00:00
        -1, // 1969-12-31 23:59:59.999
        -1500000000, // 1969-12-31 23:59:58.500
        INT64_MAX, // 2262-04-11 23:47:16.854
    };
    // For assertion
    char datetime[7][24] = {
        "1970-01-01 00:00:00",
        "2026-11-08 00:33:12",
        "2024-02-29 12:34:56",
        "2100-03-01 00:00:00",
        "1969-12-31 23:59:59",
        "1969-12-31 23:59:58",
    };
    char nanos[7][11] = {
        ".000000000",
        ".674863265",
        ".000000000",
        ".000000000",
        ".999999999",
        ".500000000",
        ".854775807",
    };


    char buffer[64];
    for (int i = 0; i < 7; i++) {
        // Ideally you would check the return value to see if it actually wrote anything
        gc_wall_format(timestamps[i], buffer, sizeof(buffer), "%a %d %b %Y, %I:%M%p", true, true);
        assert(strstr(buffer, nanos[i]) != NULL); // Verify the nanosecond part
        fprintf(stderr, "%s\n", buffer);

        gc_wall_format(timestamps[i], buffer, sizeof(buffer), "%F %I:%M%p", true, false);
        fprintf(stderr, "%s\n", buffer);

        gc_wall_format(timestamps[i], buffer, sizeof(buffer), "%F %H:%M:%S", true, false);
        assert(strstr(buffer, datetime[i]) != NULL); // Verify the date and time
        fprintf(stderr, "%s\n\n", buffer);
    }

    gc_datetime dt = gc_wall_to_datetime(timestamps[0], true);
    gc_datetime dt_local = gc_wall_to_datetime(timestamps[0], false);

    printf("UTC Unix Epoch:\n"
           "Year (offset from 1900): %d\n"
           "Month(s since January): %d\n"
           "Day of the month: %d\n"
           "Hour: %d\n"
           "Minute: %d\n"
           "Second(up to 60 for leap seconds): %d\n"
           "Nanoseconds: %d\n"
           "Day of the week (0-6 Sunday-Saturday): %d\n"
           "UTC Offset (in seconds east of UTC): %ld\n\n",
           dt.year, dt.month, dt.day, dt.hours, dt.minutes, dt.seconds, dt.nanoseconds, dt.weekday, dt.utc_offset );

    printf("Local Time Zone Unix Epoch:\n"
           "Year (offset from 1900): %d\n"
           "Month(s since January): %d\n"
           "Day of the month: %d\n"
           "Hour: %d\n"
           "Minute: %d\n"
           "Second(up to 60 for leap seconds): %d\n"
           "Nanoseconds: %d\n"
           "Day of the week (0-6 Sunday-Saturday): %d\n"
           "UTC Offset (in seconds east of UTC): %ld\n\n",
           dt_local.year, dt_local.month, dt_local.day, dt_local.hours, dt_local.minutes, dt_local.seconds,
           dt_local.nanoseconds, dt_local.weekday, dt_local.utc_offset );

    return 0;
}
