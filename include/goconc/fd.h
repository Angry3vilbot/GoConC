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

#ifndef GOCONC_FD_H
#define GOCONC_FD_H
#include <stddef.h>
#include <unistd.h>

/// File Descriptor
typedef struct gc_fd gc_fd;

/// Wrap a file descriptor
/// @details Stores the file descriptor inside a newly allocated gc_fd, together with a new
/// @link gc_polldesc @endlink. Initializes the poller if it isn't already initialized.
/// @note Sets O_NONBLOCK on the file descriptor if not already set.
/// @note Check the error number using errno if you need the reason for failure.
/// @param fd The file descriptor to wrap
/// @return The pointer to the allocated @link gc_fd @endlink, nullptr on failure
gc_fd* gc_fd_wrap(int fd);

/// Read the specified number of bytes into the given buffer from a file descriptor.
/// @note Check the error number using errno if you need the reason for failure.
/// @param fd The @link gc_fd @endlink to read on
/// @param buffer The buffer in which to store the read bytes
/// @param count The number of bytes to read
/// @return The number of bytes read, -1 on failure
ssize_t gc_fd_read(gc_fd *fd, void *buffer, size_t count);

/// Write the specified number of bytes from a given buffer into a file descriptor.
/// @note Check the error number using errno if you need the reason for failure.
/// @param fd The @link gc_fd @endlink to write on
/// @param buffer The buffer from which to write
/// @param count The number of bytes to write
/// @return The number of bytes written, -1 on failure
ssize_t gc_fd_write(gc_fd *fd, const void *buffer, size_t count);

/// Close a file descriptor
/// @details Wakes any goroutines that are waiting to read/write. The last user of the file descriptor
/// will then free the memory allocated to it and the inner @link gc_polldesc @endlink
/// @note Check the error number using errno if you need the reason for failure.
/// @param fd The @link gc_fd @endlink to close
/// @return 0 on success, -1 on failure
int gc_fd_close(gc_fd *fd);

#endif //GOCONC_FD_H
