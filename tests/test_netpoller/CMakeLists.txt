set(
        tests
        descriptors_and_init
        pipe
)

foreach (name IN LISTS tests)
    add_executable("test_netpoller_${name}" "test_${name}.c")
    target_link_libraries("test_netpoller_${name}" PRIVATE GoConC::GoConC)
    target_include_directories("test_netpoller_${name}" PRIVATE ${PROJECT_SOURCE_DIR}/src)
    add_test(NAME "${name}" COMMAND "test_netpoller_${name}")
endforeach ()