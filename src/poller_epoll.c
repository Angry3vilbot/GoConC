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

#include <sys/epoll.h>

#include "poller.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// File descriptor of the poller, assigned by @link epoll_create1 @endlink.
/// Default -1 means the poller is not initialised and the scheduler loop will skip polling.
static int gc_epoll_fd = -1;

int gc_poller_init() {
    if (gc_poller_inited()) return 0;
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    // Error creating an epoll instance
    if (epoll_fd == -1) return -1;

    gc_epoll_fd = epoll_fd;
    return 0;
}

bool gc_poller_inited() {
    return gc_epoll_fd != -1;
}

int gc_poller_add(gc_polldesc* pd) {
    // Create the event
    struct epoll_event event;
    event.events = EPOLLIN | EPOLLOUT | EPOLLRDHUP | EPOLLET;
    // Store pd as the user data
    event.data.ptr = pd;
    // Add the entry to the epoll interest list
    int retv = epoll_ctl(gc_epoll_fd, EPOLL_CTL_ADD, pd->file_descriptor, &event);
    if (retv == -1) {
        return -1;
    }
    // If the operation succeeded, set the poll descriptor's registered flag
    pd->registered = true;
    return 0;
}

int gc_poller_del(gc_polldesc* pd) {
    if (!pd->registered) return 0;

    struct epoll_event event;
    int retv = epoll_ctl(gc_epoll_fd, EPOLL_CTL_DEL, pd->file_descriptor, &event);

    pd->registered = false;
    return retv;
}

int gc_poller_poll(gc_duration delay) {
    int timeout;
    // Get the timeout from the given delay
    // Negative delay means wait forever
    if (delay < 0) {
        timeout = -1;
    }
    // Delay of zero means check then return immediately
    else if (delay == 0) {
        timeout = 0;
    }
    else {
        // Convert the nanoseconds of gc_duration into milliseconds used by epoll_wait
        // Round up the result because we cannot wait less than the given delay
        int64_t seconds = (delay + GC_MILLISECOND - 1) / GC_MILLISECOND;
        if (seconds > INT_MAX) seconds = INT_MAX;
        timeout = seconds;
    }
    // Create a buffer for the available events
    struct epoll_event events[GC_POLLER_EVENT_BUFFER_SIZE];
    // Amount of goroutines woken up by the poll
    int delta = 0;
    int count;
    do {
        count = epoll_wait(gc_epoll_fd, events, GC_POLLER_EVENT_BUFFER_SIZE, timeout);
        if (count < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "gc_poller_poll: epoll_wait failed: %s\n", strerror(errno));
                abort();
            }
            // If we must return a result immediately, we should retry
            if (timeout == 0) continue;
            return delta;
        }
        // Loop over every available event to ready the goroutines waiting on them
        for (int i = 0; i < count; i++) {
            struct epoll_event event = events[i];
            bool isReadable = event.events & (EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR);
            bool isWritable = event.events & (EPOLLOUT | EPOLLHUP | EPOLLERR);

            gc_polldesc* pd = event.data.ptr;
            delta += gc_poller_ready(pd, isReadable, isWritable);
        }
        // If the buffer was full, retry again because we might've missed some events
        // Set the timeout to 0 to fetch the following events without one
        if (count == GC_POLLER_EVENT_BUFFER_SIZE) {
            timeout = 0;
        }
    } while (count == GC_POLLER_EVENT_BUFFER_SIZE);

    return delta;
}