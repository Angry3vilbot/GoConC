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
    waiter->queued = true;
}

/// Check if the waiter is stale
/// @details Checks whether the waiter is stale.
/// The only time a waiter becomes stale is when it loses in a select.
/// A winning case's waiter sets the index inside select_state using this function.
/// @param waiter the waiter to check
/// @return <code>true</code> if the waiter is stale, <code>false</code> otherwise.
bool gc_waiter_is_stale(gc_waiter *waiter) {
    // Plain waiters are always valid
    if (waiter->select_state == NULL) return false;
    // If the select state contains an index, it means a different case has already won, so the waiter is stale
    if (waiter->select_state->index != -1) return true;
    // Set the select state's index to the winning case's index
    waiter->select_state->index = waiter->select_case_index;
    return false;
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
    head->queued = false;
    return head;
}

/// Dequeue waiters until one can be claimed.
/// @details Skips any stale waiters.
/// @param queue the queue to dequeue from
/// @return the pointer to the dequeued <code>gc_waiter</code>, <code>nullptr</code> if none was found
gc_waiter* gc_wait_queue_dequeue_claimed(gc_wait_queue *queue) {
    while (!gc_wait_queue_isEmpty(queue)) {
        gc_waiter* waiter = gc_wait_queue_dequeue(queue);
        if (!gc_waiter_is_stale(waiter)) {
            return waiter;
        }
    }
    return nullptr;
}

/// Remove a waiter from the queue
/// @param queue the queue to remove the waiter from
/// @param waiter the waiter to remove
void gc_wait_queue_remove(gc_wait_queue *queue, gc_waiter *waiter) {
    gc_waiter* current = queue->head;
    gc_waiter* prev = nullptr;
    // TODO: Add a prev pointer to waiters for O(1) removal if the queue walking becomes an issue
    while (current != nullptr) {
        if (current == waiter) {
            // If the waiter is not at head
            if (prev != nullptr) {
                prev->next = waiter->next;
            }
            // If the waiter is at head
            else {
                queue->head = waiter->next;
            }
            // If the waiter is at tail
            if (waiter->next == nullptr) {
                queue->tail = prev;
            }
            // Just in case
            waiter->next = nullptr;
            waiter->queued = false;
            return;
        }
        // Walk the queue
        prev = current;
        current = current->next;
    }
}