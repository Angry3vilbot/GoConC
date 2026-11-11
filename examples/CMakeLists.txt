set(
        examples
        concurrent_prime_sieve
        dining_philosophers
        worker_pools
        rate_limiting
)

foreach (example IN LISTS examples)
    add_executable("${example}_example" "${example}.c")
    target_include_directories("${example}_example" PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries("${example}_example" PRIVATE GoConC::GoConC GoConC::main)
endforeach ()


add_executable(concurrent_pi_example concurrent_pi.c)
target_include_directories(concurrent_pi_example PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(concurrent_pi_example PRIVATE GoConC::GoConC GoConC::main)
# libm is needed for pow() on Linux; macOS and Windows have it built in
if(UNIX AND NOT APPLE)
    target_link_libraries(concurrent_pi_example PRIVATE m)
endif()