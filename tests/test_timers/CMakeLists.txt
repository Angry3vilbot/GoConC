set(
        tests
        sleep
        after
        ticker
)

foreach (name IN LISTS tests)
    add_executable("test_${name}" "test_${name}.c")
    target_link_libraries("test_${name}" PRIVATE GoConC::GoConC)
    add_test(NAME "${name}" COMMAND "test_${name}")
endforeach ()