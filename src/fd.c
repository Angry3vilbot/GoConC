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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <bits/signum-generic.h>

#include "poller.h"
#include "scheduler_p.h"
#include "fd_p.h"
#include "poll_p.h"
#include "goconc/fd.h"

/// Called once at the start of gc_fd_wrap to ignore SIGPIPE so the program doesn't die
/// when writing to a closed pipe
static void gc_ignore_sigpipe() {
    static bool done = false;
    if (!done) {
        done = true;
        sigaction(SIGPIPE, NULL, NULL);
    }
}

/// Called before completing an operation (Read/Write/Close) on a gc_fd to free it
/// if this is the last reference
/// @param fd The gc_fd to leave.
static void gc_fd_leave(gc_fd* fd) {
    fd->active_ref_count--;
    if (fd->active_ref_count == 0 && fd->should_free) {
        // Save the previous errno
        int err = errno;
        gc_pd_release(fd->poller_descriptor);
        free(fd);
        // And restore it
        errno = err;
    }
}

gc_fd* gc_fd_wrap(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) return nullptr;
    // Set O_NONBLOCK if it isn't already set
    if (!(flags & O_NONBLOCK)) {
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) return nullptr;
    }
    // Prevent the program from killing itself if it writes to a closed pipe
    gc_ignore_sigpipe();
    // Initialise the poller if it isn't already initialised
    if (gc_poller_init() == -1) {
        // Preserve the error in case fcntl fails
        int err = errno;
        fprintf(stderr, "GoConC Runtime Error - gc_fd_wrap: Failed to initialise the poller. %s\n",
            strerror(errno));
        fcntl(fd, F_SETFL, flags);
        // Restore the error
        errno = err;
        return nullptr;
    }
    // Get a poll descriptor from the pool
    gc_polldesc* polldesc = gc_pd_alloc();
    if (polldesc == NULL) {
        fcntl(fd, F_SETFL, flags);
        return nullptr;
    }
    polldesc->file_descriptor = fd;

    // Add the poll descriptor to the poller
    if (gc_poller_add(polldesc) == -1) {
        // Preserve the error in case fcntl fails
        int err = errno;
        fprintf(stderr, "GoConC Runtime Error - gc_fd_wrap: Failed to add the poll descriptor. %s\n",
            strerror(errno));
        fcntl(fd, F_SETFL, flags);
        gc_pd_release(polldesc);
        // Restore the error
        errno = err;
        return nullptr;
    }
    // Allocate the file descriptor
    gc_fd* file_descriptor = calloc(1, sizeof(gc_fd));
    if (file_descriptor == NULL) {
        fcntl(fd, F_SETFL, flags);
        return nullptr;
    }
    file_descriptor->file_descriptor = fd;
    file_descriptor->poller_descriptor = polldesc;
    return file_descriptor;
}

ssize_t gc_fd_read(gc_fd *fd, void *buffer, size_t count) {
    if (fd == NULL) {
        fprintf(stderr, "GoConC Fatal Error: null gc_fd in gc_fd_read.\n");
        abort();
    }
    gc_scheduler* scheduler = gc_sched_get();
    if (scheduler->current == NULL) {
        fprintf(stderr, "GoConC Fatal Error: gc_fd_read called outside of a goroutine.\n");
        abort();
    }

    fd->active_ref_count++;
    while (true) {
        // If the descriptor is closing, return
        if (fd->poller_descriptor->closing) {
            // Set the error number to Bad File Descriptor
            errno = EBADF;
            fd->active_ref_count--;
            return -1;
        }
        ssize_t retv = read(fd->file_descriptor, buffer, count);
        // On success, return the number of bytes read
        if (retv >= 0) {
            fd->active_ref_count--;
            return retv;
        }
        // Otherwise, check the error number
        int err = errno;
        switch (err) {
            case EINTR:
                // Try again
                continue;
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
                // On EAGAIN and EWOULDBLOCK, try waiting
                if (!gc_poll_wait_read(fd->poller_descriptor)) {
                    // Set the error number
                    // TODO: After timeouts, set to ETIMEDOUT on that
                    errno = EBADF;
                    fd->active_ref_count--;
                    return -1;
                }
                // Now, try again
                continue;
            default:
                errno = err;
                fd->active_ref_count--;
                return -1;
        }
    }
}

ssize_t gc_fd_write(gc_fd *fd, const void *buffer, size_t count) {
    if (fd == NULL) {
        fprintf(stderr, "GoConC Fatal Error: null gc_fd in gc_fd_write.\n");
        abort();
    }
    gc_scheduler* scheduler = gc_sched_get();
    if (scheduler->current == NULL) {
        fprintf(stderr, "GoConC Fatal Error: gc_fd_write called outside of a goroutine.\n");
        abort();
    }

    fd->active_ref_count++;
    const char* pointer = buffer;
    // Keep trying to write starting at this offset until all data is written
    size_t bytes_written = 0;
    while (bytes_written < count) {
        // If the descriptor is closing, return
        if (fd->poller_descriptor->closing) {
            // Set the error number to Bad File Descriptor
            errno = EBADF;
            gc_fd_leave(fd);
            return bytes_written ? (ssize_t)bytes_written : -1;
        }
        ssize_t n = write(fd->file_descriptor, pointer + bytes_written, count - bytes_written);
        // On success, save the amount of bytes written and restart the loop
        if (n > 0) {
            bytes_written += (size_t)n;
            continue;
        }
        // If we wrote nothing when count > 0, treat it as an error so the loop doesn't spin endlessly
        if (n == 0) {
            errno = EIO;
            gc_fd_leave(fd);
            return bytes_written ? (ssize_t)bytes_written : -1;
        }
        // Otherwise, check the error number
        int err = errno;
        switch (err) {
            case EINTR:
                // Try again
                continue;
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
                // On EAGAIN and EWOULDBLOCK, try waiting
                if (!gc_poll_wait_write(fd->poller_descriptor)) {
                    // Set the error number
                    // TODO: After timeouts, set to ETIMEDOUT on that
                    errno = EBADF;
                    gc_fd_leave(fd);
                    return bytes_written ? (ssize_t)bytes_written : -1;
                }
                // Now, try again
                continue;
            default:
                errno = err;
                gc_fd_leave(fd);
                return bytes_written ? (ssize_t)bytes_written : -1;
        }
    }
    fd->active_ref_count--;
    return (ssize_t)bytes_written;
}

int gc_fd_close(gc_fd *fd) {
    if (fd == NULL) {
        fprintf(stderr, "GoConC Fatal Error: null gc_fd in gc_fd_close.\n");
        abort();
    }
    // If the poller descriptor is closing already, error out to prevent a double close
    if (fd->poller_descriptor->closing) {
        errno = EBADF;
        return -1;
    }
    // Set the poller descriptor's closing flag
    fd->poller_descriptor->closing = true;
    // Wake up both waiters (if they exist)
    if (fd->poller_descriptor->read_state == GC_PD_WAIT) gc_pd_unblock(fd->poller_descriptor, 0);
    if (fd->poller_descriptor->write_state == GC_PD_WAIT) gc_pd_unblock(fd->poller_descriptor, 1);
    // Set the state to NULL to prevent stale READY state causing an abort
    fd->poller_descriptor->read_state = GC_PD_NULL;
    fd->poller_descriptor->write_state = GC_PD_NULL;
    // Close the file descriptor and remove the poller descriptor
    gc_poller_del(fd->poller_descriptor);
    int retv = close(fd->file_descriptor);
    // If there are any goroutines using this gc_fd, mark it as freeable and return
    if (fd->active_ref_count > 0) {
        fd->should_free = true;
        return retv;
    }
    // If there aren't we can free it here
    // Save the errno from close()
    int err = errno;
    gc_pd_release(fd->poller_descriptor);
    free(fd);
    errno = err;
    return retv;
}