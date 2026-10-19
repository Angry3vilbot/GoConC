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

#ifndef GOCONC_CHANNEL_H
#define GOCONC_CHANNEL_H

#include <stddef.h>

typedef struct gc_channel gc_channel;

gc_channel* gc_make(size_t unit_size, int buffer_size);
void gc_free(gc_channel* chan);
int gc_send(gc_channel *chan, void *data);
int gc_receive(gc_channel *chan, void *data);
void gc_close(gc_channel *chan);
// Status codes
constexpr int GC_CHANNEL_OK = 0;
constexpr int GC_CHANNEL_ERROR = 1;
constexpr int GC_CHANNEL_CLOSED = 2;
constexpr int GC_CHANNEL_WOULD_BLOCK = 3;

#endif //GOCONC_CHANNEL_H
