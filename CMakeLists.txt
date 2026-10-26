cmake_minimum_required(VERSION 3.21)
project(GoConC VERSION 0.0.1 LANGUAGES C)

# Options
option(GOCONC_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(GOCONC_SANITIZE "Build with ASan/UBSan (not TSan)" OFF)
option(GOCONC_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

set(CMAKE_C_STANDARD 23)

# Pick platform sources
string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _proc)
if(_proc MATCHES "^(x86_64|amd64)$" AND NOT WIN32)
    enable_language(ASM)
    set(GOCONC_ARCH_SRC
            src/arch/ctx_x86_64_sysv.S
            src/arch/ctx_make_x86_64_sysv.c)
elseif(_proc MATCHES "^(aarch64|arm64)$")
    message(FATAL_ERROR "AArch64 not implemented yet")
else()
    message(FATAL_ERROR "Unsupported platform: ${CMAKE_SYSTEM_NAME}/${_proc}")
endif()

if(GOCONC_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

add_library(GoConC STATIC
        src/stack_posix.c
        src/scheduler.c
        src/run_queue.c
        src/waiter.c
        src/channel.c
        src/waitgroup.c
        src/mutex.c
        ${GOCONC_ARCH_SRC})
add_library(GoConC::GoConC ALIAS GoConC)

target_include_directories(GoConC
        PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE src)
target_compile_definitions(GoConC PRIVATE _DEFAULT_SOURCE)

if(GOCONC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GOCONC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()