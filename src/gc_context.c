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

#include "goconc/gc_context.h"

#include <stdio.h>
#include <stdlib.h>

#include "gc_context_p.h"

static gc_context gc_empty_context = { .type = GC_CONTEXT_BACKGROUND };

/// Walk the sibling linked list to find the last immediate child of a parent context
/// @param parent The parent context whose children to look through
/// @return The pointer to said child
gc_context* get_last_child(gc_context* parent) {
    gc_context* current = parent->children;
    while (current != NULL) {
        if (current->next_sibling == NULL) {
            return current;
        }
        current = current->next_sibling;
    }
    return nullptr;
}

/// Walk the parent linked list to find the nearest cancellable ancestor to anchor to
/// @param parent The parent of the new context
/// @return The pointer to the nearest cancellable ancestor
gc_context* find_cancellable_parent(gc_context* parent) {
    gc_context* current = parent;
    while (current != NULL) {
        if (current->type == GC_CONTEXT_CANCEL || current->type == GC_CONTEXT_TIMER) {
            return current;
        }
        current = current->parent;
    }
    return nullptr;
}

gc_context* gc_context_make(gc_context* parent) {
    if (parent == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: Cannot create a context with a null parent\n");
        abort();
    }
    // Allocate the context
    gc_context* context = calloc(1, sizeof(gc_context));
    if (context == NULL) {
        return nullptr;
    }
    // Create the Done channel
    context->done = gc_make(sizeof(unsigned char), 0);
    if (context->done == NULL) {
        free(context);
        return nullptr;
    }
    // Link the new context to the parent
    context->parent = parent;
    // Find the nearest cancellable ancestor and save it
    context->cancel_parent = find_cancellable_parent(context->parent);

    return context;
}

void gc_context_free(gc_context* context) {
    // Null and static contexts cannot be freed, noop
    if (context == NULL || context->type == GC_CONTEXT_BACKGROUND) return;
    // Value contexts simply have their struct freed
    if (context->type == GC_CONTEXT_VALUE) {
        free(context);
        return;
    }
    // Cancel the context first if it's not cancelled already
    if (context->error == GC_CONTEXT_NONE) gc_context_cancel(context);
    // Free the timer if one exists
    if (context->type == GC_CONTEXT_TIMER) gc_timer_free(context->timer.timer);
    // Free the Done channel and the struct
    gc_free(context->done);
    free(context);
}

/// Walks the ancestor list to find the deadline of a tree of contexts if there is one.
/// @param parent The parent context to start the lookup chain from
/// @return The shared earliest deadline if set, <code>nullptr</code> if there isn't one
gc_mono_time* gc_context_get_deadline(gc_context* parent) {
    gc_mono_time* result = nullptr;

    gc_context* current = parent;
    while (current->parent != NULL) {
        if (current->type == GC_CONTEXT_TIMER) {
            result = &current->timer.deadline;
            break;
        }
        current = current->parent;
    }

    return result;
}

gc_context* gc_background_context() {
    return &gc_empty_context;
}

gc_context* gc_todo_context() {
    return gc_background_context();
}

gc_context* gc_context_with_cancel(gc_context* parent) {
    gc_context* context = gc_context_make(parent);
    if (context == NULL) {
        return nullptr;
    }
    // If the parent is already cancelled, cancel the context immediately
    if (parent->error != GC_CONTEXT_NONE) {
        // Do it manually since it doesn't have (nor does it need) a type
        context->error = parent->error;
        gc_close(context->done);
        context->cancel_parent = nullptr;

        return context;
    }
    // Link the new context to its siblings
    gc_context* ancestor = find_cancellable_parent(parent);
    if (ancestor != NULL) {
        context->next_sibling = ancestor->children;
        ancestor->children = context;
    }
    // Set the type for gc_context_cancel
    context->type = GC_CONTEXT_CANCEL;

    return context;
}

gc_context* gc_context_with_deadline(gc_context* parent, gc_mono_time deadline) {
    gc_context* context = gc_context_make(parent);
    if (context == NULL) {
        return nullptr;
    }
    // If the parent is already cancelled, cancel the context immediately
    if (parent->error != GC_CONTEXT_NONE) {
        // Do it manually since it doesn't have (nor does it need) a type
        context->error = parent->error;
        gc_close(context->done);
        context->cancel_parent = nullptr;

        return context;
    }
    // If the parent's deadline is earlier than the context's, create it as a cancel context
    gc_mono_time* effective_deadline = gc_context_get_deadline(parent);
    if (effective_deadline != NULL && *effective_deadline <= deadline) {
        // Set the type for gc_context_cancel
        context->type = GC_CONTEXT_CANCEL;
        return context;
    }
    // If the deadline already passed, cancel it immediately
    if (deadline <= gc_monotonic_time_now()) {
        context->error = GC_CONTEXT_DEADLINE_EXCEEDED;
        gc_close(context->done);
        context->cancel_parent = nullptr;

        return context;
    }
    // Create the timer, this will be the earliest timer for any future children
    gc_timer* timer = gc_context_after(deadline, gc_context_deadline_cancel, context);
    if (timer == NULL) {
        gc_free(context->done);
        free(context);
        return nullptr;
    }
    // Link the new context to its siblings
    gc_context* last_sibling = get_last_child(parent);
    if (last_sibling == NULL) {
        parent->children = context;
    }
    else {
        last_sibling->next_sibling = context;
    }
    // Set the type for gc_context_cancel
    context->type = GC_CONTEXT_TIMER;

    return context;
}

gc_context* gc_context_with_timeout(gc_context* parent, gc_duration timeout) {
    return gc_context_with_deadline(parent, gc_monotonic_time_add(gc_monotonic_time_now(), timeout));
}

gc_context* gc_context_with_value(gc_context* parent, void* key, void* value) {
    if (key == NULL) {
        fprintf(stderr, "GoConC Fatal Error: Value Context key cannot be null\n");
        abort();
    }
    if (parent == NULL) {
        fprintf(stderr, "GoConC Fatal Error: Cannot create a context with a null parent\n");
        abort();
    }
    // Allocate the context
    gc_context* context = calloc(1, sizeof(gc_context));
    if (context == NULL) {
        return nullptr;
    }
    context->type = GC_CONTEXT_VALUE;
    context->parent = parent;
    context->value.key = key;
    context->value.value = value;
    context->cancel_parent = find_cancellable_parent(parent);

    return context;
}

void cancel_with_error(gc_context* context, int error, bool detach_from_parent) {
    if (context == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: Cannot cancel a null context\n");
        abort();
    }
    // If the context was already cancelled, return
    if (context->error != GC_CONTEXT_NONE) return;
    // Set the error code
    context->error = error;
    // Close the Done channel
    gc_close(context->done);
    // If it's a deadline/timeout context, stop the timer if it's not stopped already
    if (context->type == GC_CONTEXT_TIMER && context->timer.timer != NULL) {
        gc_timer_stop(context->timer.timer);
    }
    // Cancel every child with the same error code.
    gc_context* child = context->children;
    // Detach the list first
    context->children = nullptr;
    while (child != NULL) {
        gc_context* sibling = child->next_sibling;
        child->next_sibling = nullptr;
        child->cancel_parent = nullptr;
        cancel_with_error(child, error, false);
        child = sibling;
    }
    // Detach from the parent
    if (detach_from_parent) {
        gc_context* parent = context->cancel_parent;
        if (parent == NULL) return;

        gc_context** link = &parent->children;
        // Walk using a pointer to a pointer, if it points to context we're at the previous node
        while (*link != NULL) {
            if (*link == context) {
                *link = context->next_sibling;
                context->next_sibling = nullptr;
                return;
            }
            link = &(*link)->next_sibling;
        }
    }
}
void gc_context_cancel(gc_context* context) {
    cancel_with_error(context, GC_CONTEXT_CANCELLED, true);
}
void gc_context_deadline_cancel(void *arg) {
    gc_context* context = arg;
    cancel_with_error(context, GC_CONTEXT_DEADLINE_EXCEEDED, true);
}

gc_channel* gc_context_done(gc_context* context) {
    if (context == nullptr || context->type == GC_CONTEXT_BACKGROUND) {
        return nullptr;
    }
    if (context->type == GC_CONTEXT_VALUE) {
        return context->cancel_parent ? context->cancel_parent->done : nullptr;
    }
    return context->done;
}

int gc_context_error(gc_context* context) {
    if (context == NULL) {
        fprintf(stderr, "GoConC Fatal Error: null context cannot have an error code\n");
        abort();
    }
    if (context->type == GC_CONTEXT_VALUE) {
        return context->cancel_parent ? context->cancel_parent->error : 0;
    }
    return context->error;
}

void* gc_context_value(gc_context* context, void* key) {
    if (context == nullptr) {
        fprintf(stderr, "GoConC Fatal Error: null context cannot have a value\n");
        abort();
    }

    for (gc_context* ctx = context; ctx != NULL; ctx = ctx->parent) {
        if (ctx->type == GC_CONTEXT_VALUE && ctx->value.key == key) {
            return ctx->value.value;
        }
    }

    return nullptr;
}
