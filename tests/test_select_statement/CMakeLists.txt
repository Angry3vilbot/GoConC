set(
        tests
        select
        select_race
        select_close
        select_default
)

foreach (name IN LISTS tests)
    add_executable("test_${name}" "test_${name}.c")
    target_include_directories("test_${name}" PRIVATE ${PROJECT_SOURCE_DIR}/src)
    target_link_libraries("test_${name}" PRIVATE GoConC::GoConC)
    add_test(NAME "${name}" COMMAND "test_${name}")
endforeach ()