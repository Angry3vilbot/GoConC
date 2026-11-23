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
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "goconc.h"
// Test reading from/writing to a pipe

void reader(void* arg) {
    gc_fd* fd = arg;
    // Create the buffer for reading
    char buf[1024];
    // Read from the pipe
    ssize_t bytes = gc_fd_read(fd, &buf, sizeof("Hello World"));
    if (bytes != -1) {
        fprintf(stderr, "Read %ld bytes\n", bytes);
        // Limit printf to only printing the returned size
        fprintf(stderr, "%.*s\n", (int)bytes, buf);
    }
    // Test reading before a write
    bytes = gc_fd_read(fd, &buf, sizeof("Write after Read"));
    if (bytes != -1) {
        fprintf(stderr, "Read %ld bytes\n", bytes);
        fprintf(stderr, "%.*s\n", (int)bytes, buf);
    }
    // Yield to the writer
    gc_Gosched();
    // Test reading the data in chunks
    int running_offset = 0;
    for (int i = 0; i < 4; i++) {
        bytes = gc_fd_read(fd, buf + running_offset, 6);
        if (bytes != -1) {
            fprintf(stderr, "Read %ld bytes\n", bytes);
            fprintf(stderr, "%.*s\n", (int)bytes + running_offset, buf);
            running_offset += (int)bytes;
        }
    }
    // Test zero length read
    bytes = gc_fd_read(fd, &buf, 0);
    fprintf(stderr, "'Read' %ld bytes\n", bytes);
    // Yield to the writer
    gc_Gosched();
    // Test writing more than the pipe buffer holds
    unsigned char *pattern = malloc(1 << 20);
    for (int i = 0; i < (1 << 20); i++) {
        pattern[i] = (unsigned char)(i * 31 + i / 251);
    }

    unsigned char *test = malloc(1 << 20);
    ssize_t offset = 0;
    // Loop until EOF
    while (true) {
        ssize_t bytes_ssize = gc_fd_read(fd, test + offset, 4093);
        // EOF
        if (bytes_ssize == 0) break;
        // Error
        if (bytes_ssize < 0) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
            abort();
        }
        // Success, yield back to the writer so we don't just keep spinning
        fprintf(stderr, "Read %ld bytes\n", bytes_ssize);
        offset += bytes_ssize;
        gc_Gosched();
    }
    // Verify that the pattern is correct
    for (int i = 0; i < 1 << 20; i++) {
        assert(pattern[i] == test[i]);
    }
    // Try reading from a closed pipe
    bytes = gc_fd_read(fd, &buf, 20);
    fprintf(stderr, "Read %ld bytes\n", bytes);
    if (bytes == 0) fprintf(stderr, "EOF\n");
    assert(bytes == 0);
    // Close the file descriptor
    gc_fd_close(fd);
    // Cleanup
    free(pattern);
    free(test);
}

void writer(void* arg) {
    gc_fd* fd = arg;
    // Send "Hello World" into the pipe
    char string_1[] = "Hello World";
    ssize_t bytes = gc_fd_write(fd, string_1, sizeof(string_1));
    fprintf(stderr, "Writer wrote %ld bytes\n", bytes);
    // Yield to the reader
    gc_Gosched();
    // Send "Write after Read" into the pipe
    char string_2[] = "Write after Read";
    bytes = gc_fd_write(fd, string_2, sizeof(string_2));
    fprintf(stderr, "Writer wrote %ld bytes\n", bytes);
    // Yield to the poller
    gc_Gosched();
    // Send one large string for the reader to read in chunks
    char string_3[] = "Read this in chunks\n";
    bytes = gc_fd_write(fd, string_3, sizeof(string_3));
    fprintf(stderr, "Writer wrote %ld bytes\n", bytes);
    // Test zero length write
    bytes = gc_fd_write(fd, string_3, 0);
    fprintf(stderr, "Writer wrote %ld bytes\n", bytes);
    // Yield to the reader
    gc_Gosched();
    // IMPORTANT!!! Send again to remove the cached read for 3 bytes
    // otherwise it consumes them from the following test
    gc_fd_write(fd, &"123", 3);
    // Test writing more than the pipe buffer holds
    unsigned char *pattern = malloc(1 << 20);
    for (int i = 0; i < (1 << 20); i++) {
        pattern[i] = (unsigned char)(i * 31 + i / 251);
    }
    bytes = gc_fd_write(fd, pattern, 1 << 20);
    fprintf(stderr, "Writer wrote %ld bytes\n", bytes);
    // Close the file descriptor
    gc_fd_close(fd);
    // Cleanup
    free(pattern);
}

void test(void* arg) {
    // Create the pipe
    int pipefd[2];
    pipe(pipefd);
    // Wrap both ends
    gc_fd* read_fd = gc_fd_wrap(pipefd[0]);
    gc_fd* write_fd = gc_fd_wrap(pipefd[1]);
    // Start the reader and writer
    gc_go(writer, write_fd);
    gc_go(reader, read_fd);
}

int main() {
    gc_go(test, nullptr);
    gc_run();
    return 0;
}