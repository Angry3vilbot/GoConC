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

#ifndef GOCONC_GC_CONTEXT_H
#define GOCONC_GC_CONTEXT_H
#include "channel.h"
#include "datetime.h"

typedef struct gc_context gc_context;

/// Create an empty context.
/// @details A background context is used as a top-level context (the root of the tree).
/// It is never cancelled, has no values and no deadline.
/// @return A pointer to an empty <code>gc_context</code>
gc_context* gc_background_context();

/// Create an empty TODO context.
/// @details A TODO context is used as a placeholder for when you don't yet know which context you need to use,
/// or if one isn't available yet.\n\n The difference between this and a background context is the fact that the
/// usage of <code>gc_todo_context()</code> shows a warning to remind you to swap it out for a different type.
/// @return A pointer to an empty <code>gc_context</code>
gc_context* gc_todo_context()__attribute__((warning("TODO Context")));

/// Create a new context with its own channel.
/// @details Creates a <code>gc_context</code> that points to the provided parent context,
/// but has its own <code>done</code> channel.\n\n This context's <code>done</code> channel gets closed when
/// @link gc_context_cancel @endlink is called with it as the argument, or when its parent context's
/// <code>done</code> channel is closed.
/// @param parent The parent context to attach it to
/// @return The pointer to the created <code>gc_context</code>, <code>nullptr</code> on allocation failure
gc_context* gc_context_with_cancel(gc_context* parent);

/// Create a new context with a deadline.
/// @details Creates a <code>gc_context</code> that points to the provided parent context,
/// but has a deadline attached to it.\n\n This context's <code>done</code> channel gets closed when
/// the deadline expires, @link gc_context_cancel @endlink is called with it as the argument,
/// or when its parent context's <code>done</code> channel is closed.
/// @note If the parent context has a deadline that is earlier than the provided deadline, it will be
/// equivalent to having the same deadline as the parent.
/// @param parent The parent context to attach it to
/// @param deadline The timestamp at/after which to cancel the context
/// @return The pointer to the created <code>gc_context</code>, <code>nullptr</code> on allocation failure
gc_context* gc_context_with_deadline(gc_context* parent, gc_mono_time deadline);

/// Create a new context with a deadline at <code>gc_monotonic_time_now() + timeout</code>.
/// @details Calls @link gc_context_with_deadline @endlink with current time + timeout
/// @param parent The parent context to attach it to
/// @param timeout The timestamp at/after which to cancel the context
/// @return The pointer to the created <code>gc_context</code>, <code>nullptr</code> on allocation failure
gc_context* gc_context_with_timeout(gc_context* parent, gc_duration timeout);

/// Create a new context holding a key:value pair.
/// @details Creates a <code>gc_context</code> that points to the provided parent context.
/// This context is used to pass request-scoped data.
/// @note Both the key and the value are pointers and,
/// as such, have to outlive the context. The key should be a private static variable so as to not have any
/// collisions
/// @param parent The parent context to attach it to
/// @param key The pointer to the key
/// @param value The pointer to the value
/// @return The pointer to the created <code>gc_context</code>, <code>nullptr</code> on allocation failure
gc_context* gc_context_with_value(gc_context* parent, void* key, void* value);

/// Cancel all work done under the given context
/// @details <code>gc_context_cancel</code> does not wait until the work stops. It is idempotent
/// and can be called multiple times simultaneously. Any calls after the first do nothing.
/// @param context The context to cancel
void gc_context_cancel(gc_context* context);

/// Returns the Done channel of the given gc_context
/// @details Returns a channel that gets closed when work done on behalf of this context should be canceled.
/// Returns <code>nullptr</code> if the given gc_context can never be cancelled.\n\n
/// @link gc_context_with_cancel @endlink closes Done when it is cancelled manually.\n
/// @link gc_context_with_deadline @endlink closes Done when the deadline expires.\n
/// @link gc_context_with_timeout @endlink closes done when the timeout elapses.
/// @param context The context from which to get the Done channel
/// @return The pointer to the Done channel
gc_channel* gc_context_done(gc_context* context);

/// Returns the error code of the given gc_context
/// @details If the Done channel is not closed yet, it returns GC_CONTEXT_NONE.\n
/// If the Done channel is closed, it returns an error code explaining why:
/// - GC_CONTEXT_DEADLINE_EXCEEDED if the context's deadline has passed.
/// - GC_CONTEXT_CANCELLED if the context was cancelled for any other reason.
/// @param context The context from which to get the error code from
/// @return The error code
int gc_context_error(gc_context* context);

/// Returns the value associated with a given context for the key
/// @details Returns the pointer to the value associated with <code>key</code>,
/// or <code>nullptr</code> if no such value exists.
/// @param context The context to extract the value from
/// @param key The key for a value
/// @return The pointer to value as a <code>void*</code>, or <code>nullptr</code> if there is none.
void* gc_context_value(gc_context* context, void* key);

/// Free a context
/// @details Frees memory allocated to a <code>gc_context</code>, <b>including its Done channel</b>.
/// The Background and TODO contexts do not need to be freed.
/// @warning Must be called after you finish using the <code>gc_context</code> (or at the end of the program)
/// to free the memory allocated to it, otherwise you'll get a memory leak.\n
/// @warning Freeing a <code>gc_context</code> frees its Done channel as well, so make sure to only free it
/// AFTER all goroutines waiting on it have read and acknowledged the closure of the channel, e.g. using a
/// WaitGroup.\n
/// @warning Freeing a <code>gc_context</code> cancels it, and its children, but it doesn't free the children.
/// Those have to be freed separately.
/// @param context The context to free
void gc_context_free(gc_context* context);

// Error Codes
constexpr int GC_CONTEXT_NONE = 0;
constexpr int GC_CONTEXT_DEADLINE_EXCEEDED  = 1;
constexpr int GC_CONTEXT_CANCELLED = 2;

#endif //GOCONC_GC_CONTEXT_H
