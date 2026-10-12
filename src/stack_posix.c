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
#include "stack.h"
#include <sys/mman.h>
#include <unistd.h>

// Fallback for older systems
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#  define MAP_ANONYMOUS MAP_ANON
#endif

static long page_size = -1;
static constexpr int GUARD_PAGE_COUNT = 1;
static constexpr size_t MAX_STACK_SIZE = 1073741824;

/**
 * Allocates a set amount of space for the gc_stack.
 *
 * @param stack The gc_stack to allocate.
 * @param usable_size The usable size of the gc_stack.
 * @return 0 on success, -1 if mmap or mprotect fails.
 */
int gc_stack_allocate(gc_stack *stack, size_t usable_size) {
    // Get the page size on the first allocation
    if (page_size == -1) {
        page_size = sysconf(_SC_PAGESIZE);
        if (page_size <= 0) {
            page_size = -1;
            return -1;
        }
    }
    size_t page = (size_t)page_size;
    // Check for valid size
    if (usable_size < page || usable_size > MAX_STACK_SIZE) return -1;
    // Calculate the number of required pages, rounded up, and convert it back to size_t
    size_t rounded_size = (usable_size + page - 1) / page * page;
    // Add extra page(s) as a guard
    size_t guard_page_size = page * GUARD_PAGE_COUNT;
    size_t total = rounded_size + guard_page_size;
    // mmap the pages
    void *mmap_result = mmap(NULL, total, PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mmap_result == MAP_FAILED) {
        return -1;
    }
    // Protect the lowest page as a guard
    int mprotect_result = mprotect(mmap_result, guard_page_size, PROT_NONE);
    if (mprotect_result == -1) {
        munmap(mmap_result, total);
        return -1;
    }
    // Fill the struct with the data of the new allocation
    stack->alloc_base = mmap_result;
    stack->alloc_size = total;
    stack->stack_base = (void*)((uintptr_t)mmap_result + guard_page_size);
    stack->stack_size = rounded_size;

    return 0;
}

/// Deallocate and zero a stack
/// @param stack The stack to deallocate
void gc_stack_free(gc_stack *stack) {
    munmap(stack->alloc_base, stack->alloc_size);
    *stack = (gc_stack){0};
}