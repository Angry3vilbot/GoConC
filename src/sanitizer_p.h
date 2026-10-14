//
// Created by Mykhailo on 14.10.2026.
//

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
