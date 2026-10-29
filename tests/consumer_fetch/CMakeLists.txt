cmake_minimum_required(VERSION 3.21)
project(GoConCFetchContentConsumer LANGUAGES C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(GoConC
        GIT_REPOSITORY https://github.com/Angry3vilbot/GoConC.git
        GIT_TAG        "${GOCONC_TEST_TAG}"
)

FetchContent_MakeAvailable(GoConC)

add_executable(consumer ../consumer/main.c)
target_link_libraries(consumer PRIVATE GoConC::GoConC)