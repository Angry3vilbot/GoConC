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
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#include "goconc.h"
// Test connecting and passing data over TCP

void listener(void* args) {
    gc_fd* listener_fd = args;
    // Test connecting to each other on port 0
    gc_fd* accept_fd = gc_net_accept(listener_fd);
    if (accept_fd == NULL) {
        fprintf(stderr, "Listener: Failed to accept connection\n");
        abort();
    }
    // Receive a message from the dialer
    char buffer[1024];
    ssize_t bytes = gc_fd_read(accept_fd, buffer, sizeof("Hello There!"));
    if (bytes != -1) {
        fprintf(stderr, "Listener: Read %ld bytes\n", bytes);
        fprintf(stderr, "Listener: %.*s\n", (int)bytes, buffer);
        assert(bytes == sizeof("Hello There!"));
    }
    else {
        fprintf(stderr, "Listener: Failed to read from socket\n");
        abort();
    }
    // Send a message to the dialer
    fprintf(stderr, "Listener sending: General Kenobi!\n");
    if (gc_fd_write(accept_fd, "General Kenobi!", sizeof("General Kenobi!")) == -1) {
        fprintf(stderr, "Listener: Failed to write to socket\n");
        abort();
    }
    gc_fd_close(accept_fd);
}

void dialer(void* args) {
    int* port = args;
    // Test connecting to each other on port 0
    gc_fd* dialer_fd = gc_net_dial("127.0.0.1", *port);
    // Send a message to the listener
    fprintf(stderr, "Dialing on port %d\n", *port);
    fprintf(stderr, "Dialer sending: Hello There!\n");
    if (gc_fd_write(dialer_fd, "Hello There!", sizeof("Hello There!")) == -1) {
        fprintf(stderr, "Dialer: Failed to write to socket\n");
        abort();
    }
    // Receive a message from the listener
    char buffer[1024];
    ssize_t bytes = gc_fd_read(dialer_fd, buffer, sizeof("General Kenobi!"));
    if (bytes != -1) {
        fprintf(stderr, "Dialer: Read %ld bytes\n", bytes);
        fprintf(stderr, "Dialer: %.*s\n", (int)bytes, buffer);
        assert(bytes == sizeof("General Kenobi!"));
    }
    else {
        fprintf(stderr, "Dialer: Failed to read from socket\n");
        abort();
    }
    // Attempt to dial a connection no one is listening on
    gc_fd* empty_fd = gc_net_dial("127.0.0.1", *port + 1);
    fprintf(stderr, "Dialing on empty port %d\n", *port + 1);
    if (empty_fd == NULL) {
        fprintf(stderr, "Dialer: Failed to connect on empty port: %s\n", strerror(errno));
        assert(errno == ECONNREFUSED);
    }
    else {
        fprintf(stderr, "Dialer: Connected on a supposedly unused port????\n");
        abort();
    }

    gc_fd_close(dialer_fd);
}

int main() {
    // Create the listener first, to not have to hardcode/share a port
    gc_fd* listener_fd = gc_net_listen("127.0.0.1", 0, SOMAXCONN);
    if (listener_fd == NULL) {
        fprintf(stderr, "Failed to create listener\n");
        abort();
    }
    // Get the assigned port
    int port = gc_net_get_port(listener_fd);
    if (port == -1) {
        fprintf(stderr, "Failed to get port\n");
        abort();
    }

    gc_go(listener, listener_fd);
    gc_go(dialer, &port);

    gc_run();

    // Close the listener
    gc_fd_close(listener_fd);

    return 0;
}
