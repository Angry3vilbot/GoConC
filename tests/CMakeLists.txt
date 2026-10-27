set(
        tests
        context
        stack
        stack_edge_cases
        stack_loop
        scheduler
        gosched
        unbuffered_channel
        buffered_channel
        waitgroup
        mutex
        once
        main
)

foreach (name IN LISTS tests)
    add_executable("test_${name}" "test_${name}.c")
    target_include_directories("test_${name}" PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries("test_${name}" PRIVATE GoConC::GoConC)
    add_test(NAME "${name}" COMMAND "test_${name}")
endforeach ()

add_executable(test_context_registers
        test_context_registers/test_context_registers.c test_context_registers/check_registers.S)
add_executable(test_closing_channels test_closing_channels/test_closing_channels.c)
add_executable(test_send_after_closing_channel test_closing_channels/test_send_after_closing_channel.c)
add_executable(test_closing_closed_channel test_closing_channels/test_closing_closed_channel.c)
add_executable(test_producer_consumer test_closing_channels/test_producer_consumer.c)
add_executable(test_entry test_entry.c)
add_executable(test_entry_args test_entry_args.c)

target_include_directories(test_context_registers PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_context_registers PRIVATE GoConC::GoConC)
target_include_directories(test_closing_channels PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_closing_channels PRIVATE GoConC::GoConC)
target_include_directories(test_send_after_closing_channel PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_send_after_closing_channel PRIVATE GoConC::GoConC)
target_include_directories(test_closing_closed_channel PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_closing_closed_channel PRIVATE GoConC::GoConC)
target_include_directories(test_producer_consumer PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_producer_consumer PRIVATE GoConC::GoConC)
target_include_directories(test_entry PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_entry PRIVATE GoConC::GoConC GoConC::main)
target_include_directories(test_entry_args PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_entry_args PRIVATE GoConC::GoConC GoConC::main)

add_test(NAME context_registers COMMAND test_context_registers)
add_test(NAME closing_channels COMMAND test_closing_channels)
add_test(NAME send_after_closing_channel COMMAND test_send_after_closing_channel)
add_test(NAME closing_closed_channel COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:test_closing_closed_channel>)
set_tests_properties(closing_closed_channel PROPERTIES WILL_FAIL TRUE)
add_test(NAME producer_consumer COMMAND test_producer_consumer)
add_test(NAME entry COMMAND test_entry)
set_tests_properties(entry PROPERTIES WILL_FAIL TRUE)
add_test(NAME entry_args COMMAND test_entry_args a b c)