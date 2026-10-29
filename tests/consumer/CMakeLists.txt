cmake_minimum_required(VERSION 3.21)
project(GoConCConsumer LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(GoConC 0.1.0 CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE GoConC::GoConC)