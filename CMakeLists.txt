cmake_minimum_required(VERSION 3.21)
project(GoConC VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Options
option(GOCONC_BUILD_TESTS "Build tests" ${PROJECT_IS_TOP_LEVEL})
option(GOCONC_SANITIZE "Build with ASan/UBSan (not TSan)" OFF)
option(GOCONC_BUILD_EXAMPLES "Build examples" ${PROJECT_IS_TOP_LEVEL})

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
        src/once.c
        src/gc_main.c
        src/select.c
        ${GOCONC_ARCH_SRC})
add_library(GoConC::GoConC ALIAS GoConC)
target_compile_features(GoConC PUBLIC c_std_23)

# Optional library providing a main function that allows the user to simply define gc_user_main
# and have the whole program run inside a goroutine without any extra work.
# int gc_user_main(int argc, char **argv) {
#   // Contents of the "main" function here
# }
# It is equivalent to doing the following yourself:
# int func(void* arg) {}
# int main(int argc, char **argv) {
#   // Pack argc and argv into a struct if needed and pass it into gc_main, otherwise pass a nullptr
#   gc_main(func, nullptr)
# }
# Note: You can NOT define the main function yourself if you link this library, since it supplies one
# on its own. If you wish to have your own main function, just link the core library alone.
add_library(GoConC_main STATIC src/entry.c)
target_link_libraries(GoConC_main PUBLIC GoConC)
add_library(GoConC::main ALIAS GoConC_main)

target_include_directories(GoConC
        PUBLIC  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        PRIVATE src)
target_compile_definitions(GoConC PRIVATE _DEFAULT_SOURCE)

if(GOCONC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(GOCONC_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
# Install config
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS GoConC GoConC_main
    EXPORT GoConCTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT GoConCTargets
        FILE GoConCTargets.cmake
        NAMESPACE GoConC::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GoConC
)

write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/GoConCConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY ExactVersion
)

configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/GoConCConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/GoConCConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}/cmake/GoConC
)

install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/GoConCConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/GoConCConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/GoConC
)