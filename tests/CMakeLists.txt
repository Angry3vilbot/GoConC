add_executable(test_context test_context.c)
target_include_directories(test_context PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(test_context PRIVATE GoConC::GoConC)
add_test(NAME context COMMAND test_context)