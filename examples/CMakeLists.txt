add_executable(concurrent_pi_example concurrent_pi.c)
target_include_directories(concurrent_pi_example PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(concurrent_pi_example PRIVATE GoConC::GoConC)

# libm is needed for pow() on Linux; macOS and Windows have it built in
if(UNIX AND NOT APPLE)
    target_link_libraries(concurrent_pi_example PRIVATE m)
endif()