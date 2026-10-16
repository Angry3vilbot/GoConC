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

#ifndef GOCONC_QUEUE_H
#define GOCONC_QUEUE_H

typedef enum {
    GC_WAITER_SEND,
    GC_WAITER_RECEIVE,
} GC_WAITER_DIRECTION;

typedef struct gc_waiter {
    struct gc_goroutine *goroutine;
    struct gc_waiter *next;
    void *data;
    GC_WAITER_DIRECTION direction;
    // TODO: Add fields related to select
} gc_waiter;

typedef struct gc_wait_queue {
    gc_waiter *head;
    gc_waiter *tail;
} gc_wait_queue;

void gc_wait_queue_init(gc_wait_queue *queue);
bool gc_wait_queue_isEmpty(gc_wait_queue *queue);
void gc_wait_queue_enqueue(gc_wait_queue *queue, gc_waiter *waiter);
gc_waiter* gc_wait_queue_dequeue(gc_wait_queue *queue);

#endif //GOCONC_QUEUE_H
