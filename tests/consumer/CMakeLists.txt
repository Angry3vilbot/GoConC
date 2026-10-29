cmake_minimum_required(VERSION 3.21)
project(GoConCConsumer LANGUAGES C)

find_package(GoConC 0.1 CONFIG REQUIRED)

add_executable(consumer main.c)
target_link_libraries(consumer PRIVATE GoConC::GoConC)