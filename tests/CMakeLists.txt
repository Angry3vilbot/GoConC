add_executable(test_context test_context.c)
add_executable(test_context_registers
        test_context_registers/test_context_registers.c test_context_registers/check_registers.S)

target_include_directories(test_context PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_context PRIVATE GoConC::GoConC)
target_include_directories(test_context_registers PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_context_registers PRIVATE GoConC::GoConC)

add_test(NAME context COMMAND test_context)
add_test(NAME context_registers COMMAND test_context_registers)