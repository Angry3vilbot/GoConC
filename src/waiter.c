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

#include "waiter.h"

#include <stddef.h>

/// Initialize the wait queue.
/// @details Initializes both head and tail to null
/// @param queue the wait queue to initialize
void gc_wait_queue_init(gc_wait_queue *queue) {
    queue->head = NULL;
    queue->tail = NULL;
}

/// Check if the wait queue is empty
/// @param queue the queue to check
/// @return <code>true</code> if it is empty, <code>false</code> otherwise
bool gc_wait_queue_isEmpty(gc_wait_queue *queue) {
    return queue->head == NULL;
}

/// Enqueue a new waiter into the wait queue
/// @param queue the queue to enqueue into
/// @param waiter the waiter to enqueue
void gc_wait_queue_enqueue(gc_wait_queue *queue, gc_waiter *waiter) {
    waiter->next = nullptr;
    if (gc_wait_queue_isEmpty(queue)) {
        queue->head = waiter;
        queue->tail = waiter;
    }
    else {
        queue->tail->next = waiter;
        queue->tail = waiter;
    }
}

/// Dequeue a waiter from the wait queue
/// @param queue the queue to dequeue from
/// @return the pointer to the dequeued <code>gc_waiter</code>
gc_waiter* gc_wait_queue_dequeue(gc_wait_queue *queue) {
    if (gc_wait_queue_isEmpty(queue)) return nullptr;

    gc_waiter *head = queue->head;
    queue->head = head->next;
    if (queue->head == nullptr) {
        queue->tail = nullptr;
    }
    head->next = nullptr;
    return head;
}