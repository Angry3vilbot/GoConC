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

#ifndef GOCONC_SANITIZER_P_H
#define GOCONC_SANITIZER_P_H

#if defined(__SANITIZE_ADDRESS__)
#  define GC_ASAN 1
#elif defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define GC_ASAN 1
#  endif
#endif

#ifdef GC_ASAN
#  include <sanitizer/common_interface_defs.h>
#  include <sanitizer/asan_interface.h>
#  define GC_ASAN_START(save, bottom, size) __sanitizer_start_switch_fiber(save, bottom, size)
#  define GC_ASAN_END(save, bottom_old, size_old) __sanitizer_finish_switch_fiber(save, bottom_old, size_old)
#  define GC_ASAN_UNPOISON(base, size) __asan_unpoison_memory_region(base, size)
#else
#  define GC_ASAN_START(save, bottom, size) ((void)0)
#  define GC_ASAN_END(save, bottom_old, size_old) ((void)0)
#  define GC_ASAN_UNPOISON(base, size) ((void)0)
#endif

#endif //GOCONC_SANITIZER_P_H
