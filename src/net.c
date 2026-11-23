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

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "goconc/net.h"

#include "fd_p.h"
#include "scheduler_p.h"

/// Uses <code>accept4</code> for atomic flags on Linux/BSD, regular <code>accept</code>
/// with <code>fcntl</code> on macOS. This means that, as is, MacOS can and will leak the file
/// descriptor to a child process if it is forked between the accept and the fcntl calls in
/// a multithreaded application.
/// @return the file descriptor on success, -1 on error
static int gc_accept4_or_not(int fd) {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
    return accept4(fd, NULL, NULL, SOCK_NONBLOCK | SOCK_CLOEXEC);
#else
    int accept_fd = accept(fd, NULL, NULL);
    if (accept_fd == -1) return -1;
    fcntl(accept_fd, F_SETFD, FD_CLOEXEC);
    fcntl(accept_fd, F_SETFL, fcntl(accept_fd, F_GETFL) | O_NONBLOCK);
    return accept_fd;
#endif
}

/// Called before completing an accept on a gc_fd to free it
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

/// Create a non-blocking socket for the given ip address and port
/// TODO: Add a fallback for MacOS eventually...
/// @return The file descriptor of the socket on success, -1 on failure
static int gc_create_nb_socket(int domain, int type) {
    return socket(domain, type | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
}

/// Fill an IPv4 socket address.
/// @return 0 on success, -1 on error
static int gc_fill_socket_address(struct sockaddr_in* socket_address, const char* ip, uint16_t port) {
    // Fill the socket address struct
    socket_address->sin_family = AF_INET;
    socket_address->sin_port = htons(port);
    // Null IP address means any address
    if (ip == NULL) {
        socket_address->sin_addr.s_addr = htonl(INADDR_ANY);
        return 0;
    }
    // Convert and store the ip address from presentation to binary format
    if (inet_pton(AF_INET, ip, &socket_address->sin_addr) != 1) {
        errno = EINVAL;
        return -1;
    }
    return 0;
}

gc_fd* gc_net_listen(const char *ip, uint16_t port, int backlog) {
    // Create the socket
    int fd = gc_create_nb_socket(AF_INET, SOCK_STREAM);
    if (fd == -1) return nullptr;
    // Set SO_REUSEADDR to 1 to allow reusing the address during TIME_WAIT
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
        close(fd);
        return nullptr;
    }
    // Create and fill the socket address struct
    struct sockaddr_in socket_address = {0};
    if (gc_fill_socket_address(&socket_address, ip, port) == -1) {
        close(fd);
        return nullptr;
    }
    // Assign the name to the socket
    if (bind(fd, (struct sockaddr*)(&socket_address), sizeof(socket_address)) == -1) {
        close(fd);
        return nullptr;
    }
    // Listen on the socket
    if (listen(fd, backlog) == -1) {
        close(fd);
        return nullptr;
    }
    // Wrap the file descriptor and return it
    gc_fd* wrapper = gc_fd_wrap(fd);
    if (wrapper == nullptr) {
        close(fd);
        return nullptr;
    }
    return wrapper;
}

gc_fd* gc_net_accept(gc_fd *listener) {
    if (listener == NULL) {
        fprintf(stderr, "GoConC Fatal Error: null gc_fd in gc_net_accept.\n");
        abort();
    }
    gc_scheduler* scheduler = gc_sched_get();
    if (scheduler->current == NULL) {
        fprintf(stderr, "GoConC Fatal Error: gc_net_accept called outside of a goroutine.\n");
        abort();
    }
    // Accept the connection
    listener->active_ref_count++;
    while (true) {
        // If the descriptor is closing, return
        if (listener->poller_descriptor->closing) {
            // Set the error number to Bad File Descriptor
            errno = EBADF;
            gc_fd_leave(listener);
            return nullptr;
        }
        int fd = gc_accept4_or_not(listener->file_descriptor);
        // On success, wrap it and return
        if (fd != -1) {
            int opt = 1;
            // Set TCP_NODELAY
            setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
            gc_fd* wrapper = gc_fd_wrap(fd);
            if (wrapper == NULL) {
                int err = errno;
                close(fd);
                gc_fd_leave(listener);
                errno = err;
                return nullptr;
            }
            gc_fd_leave(listener);
            return wrapper;
        }
        // Otherwise, check the error number
        int err = errno;
        switch (err) {
            case EINTR:
                // Retry
                continue;
            case EAGAIN:
#if EWOULDBLOCK != EAGAIN
            case EWOULDBLOCK:
#endif
                // On EAGAIN and EWOULDBLOCK, try waiting
                if (!gc_poll_wait_read(listener->poller_descriptor)) {
                    errno = EBADF;
                    gc_fd_leave(listener);
                    return nullptr;
                }
                // Now, try again
                continue;
            case ECONNABORTED:
            case EPROTO:
                // Connection was closed before we could accept it
                // "it's a silly error, so try again." - Accept(), fd_unix.go, Go source code
                continue;
            case EMFILE:
            case ENFILE:
                // We've ran out of file descriptors
                // The user should wait a bit before retrying
                gc_fd_leave(listener);
                return nullptr;
            default:
                errno = err;
                gc_fd_leave(listener);
                return nullptr;
        }
    }
}

gc_fd* gc_net_dial(const char *ip, uint16_t port) {
    // Create the socket
    int fd = gc_create_nb_socket(AF_INET, SOCK_STREAM);
    if (fd == -1) return nullptr;
    // Create and fill the socket address struct
    struct sockaddr_in socket_address = {0};
    if (gc_fill_socket_address(&socket_address, ip, port) == -1) {
        close(fd);
        return nullptr;
    }
    // Wrap the file descriptor
    gc_fd* wrapper = gc_fd_wrap(fd);
    if (wrapper == NULL) {
        close(fd);
        return nullptr;
    }
    // Connect to it
    int status = connect(fd, (struct sockaddr*)(&socket_address), sizeof(socket_address));
    // On "failure", check the error code
    if (status == -1) {
        int err = errno;
        if (err == EINPROGRESS || err == EINTR) {
            while (true) {
                // Wait until it is writable
                if (!gc_poll_wait_write(wrapper->poller_descriptor)) {
                    gc_fd_close(wrapper);
                    errno = EBADF;
                    return nullptr;
                }
                // Check the socket's error code to make sure we're actually connected
                int socket_errno = 0;
                socklen_t len = sizeof(socket_errno);
                if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &socket_errno, &len) == -1) {
                    socket_errno = errno;
                }
                // If we failed to connect, error out
                if (socket_errno != 0) {
                    gc_fd_close(wrapper);
                    errno = socket_errno;
                    return nullptr;
                }
                // Otherwise, check that we've actually connected by checking the name of our peer
                struct sockaddr_in peer_address = {0};
                socklen_t peer_len = sizeof(peer_address);
                status = getpeername(fd, (struct sockaddr*)(&peer_address), &peer_len);
                // On success break out of the loop since we've connected successfully
                if (status == 0) break;
                // If the error is that we're not connected yet, continue the loop
                if (status == ENOTCONN) continue;
                // Otherwise, we failed to connect
                gc_fd_close(wrapper);
                errno = socket_errno;
                return nullptr;
            }
        }
        else {
            gc_fd_close(wrapper);
            return nullptr;
        }
    }
    return wrapper;
}

int gc_net_get_port(gc_fd* fd) {
    if (fd == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: null gc_fd in gc_net_get_port.\n");
        abort();
    }
    struct sockaddr_in socket_address = {0};
    socklen_t socket_len = sizeof(socket_address);
    if (getsockname(fd->file_descriptor, (struct sockaddr*)(&socket_address), &socket_len) == -1) {
        return -1;
    }
    return ntohs(socket_address.sin_port);
}