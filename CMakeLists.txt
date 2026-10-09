cmake_minimum_required(VERSION 4.3)
project(GoConC VERSION 0.0.1 LANGUAGES C)

# Options
option(GOCONC_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(GOCONC_SANITIZE "Build with ASan/UBSan (not TSan)" OFF)

set(CMAKE_C_STANDARD 23)

add_library(GoConC STATIC
        src/stack.c
        src/stack.h
        src/context.h
        src/arch/ctx_make_x86_64_sysv.c
        src/arch/ctx_make_aarch64.c
        src/arch/ctx_make_x86_64_win.c
        src/scheduler.c
        src/scheduler_p.h
        src/scheduler.h
        src/goroutine.h)
