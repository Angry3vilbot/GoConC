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

#ifndef GOCONC_NET_H
#define GOCONC_NET_H

#include "goconc/fd.h"

/// Listen for incoming connections on a given IP address and port
/// @note Check the error number using errno if you need the reason for failure.
/// @param ip The IP address to listen on
/// @param port The port to listen on
/// @param backlog The number of pending connection requests the queue can hold at once
/// @return A <code>gc_fd</code> wrapper for the network file descriptor on success,
/// <code>nullptr</code> on failure.
gc_fd* gc_net_listen(const char *ip, uint16_t port, int backlog);

/// Accept a pending connection from a listener's queue
/// @note Check the error number using errno if you need the reason for failure.
/// @note For example, a server loop on error should check for EMFILE and ENFILE error codes,
/// and back off for a moment, since it has ran out of file descriptors.
/// @param listener The <code>gc_fd</code> containing a network file descriptor of a listener
/// @return A <code>gc_fd</code> wrapper for the network file descriptor on success,
/// <code>nullptr</code> on failure.
gc_fd* gc_net_accept(gc_fd* listener);

/// Connect to a listener on a given IP address and port.
/// @note Check the error number using errno if you need the reason for failure.
/// @param ip The IP address to connect to
/// @param port The port on which to dial
/// @return A <code>gc_fd</code> wrapper for the network file descriptor on success,
/// <code>nullptr</code> on failure.
gc_fd* gc_net_dial(const char *ip, uint16_t port);

/// Get a port that the file descriptor is assigned to.
/// @note Check the error number using errno if you need the reason for failure.
/// @param fd The network file descriptor
/// @return The port on success, -1 on failure
int gc_net_get_port(gc_fd* fd);

#endif //GOCONC_NET_H
