file(GLOB ALIB_TEST_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/test/test_*.c")
set(ALIB_TEST_TARGETS "")

foreach(test_source IN LISTS ALIB_TEST_SOURCES)
    get_filename_component(test_target "${test_source}" NAME_WE)
    add_executable(${test_target} "${test_source}")
    set_target_properties(${test_target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${ALIB_TEST_OUTPUT_DIR}"
    )
    target_link_libraries(${test_target} PRIVATE atlan)
    alib_apply_defaults(${test_target})
    alib_apply_dev_flags(${test_target})
    add_test(NAME ${test_target} COMMAND $<TARGET_FILE:${test_target}>)
    list(APPEND ALIB_TEST_TARGETS ${test_target})
endforeach()

add_executable(test_athrd_fallback
    "${PROJECT_SOURCE_DIR}/test/test_athrd.c"
    "${PROJECT_SOURCE_DIR}/src/athrd.c"
)
set_target_properties(test_athrd_fallback PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${ALIB_TEST_OUTPUT_DIR}"
)
target_include_directories(test_athrd_fallback PRIVATE
    "${PROJECT_SOURCE_DIR}/inc"
    "${ALIB_BUILD_INCLUDE_ROOT}"
)
target_compile_definitions(test_athrd_fallback PRIVATE __STDC_NO_THREADS__=1)
target_link_libraries(test_athrd_fallback PRIVATE Threads::Threads)
alib_apply_defaults(test_athrd_fallback)
alib_apply_dev_flags(test_athrd_fallback)
alib_apply_platform(test_athrd_fallback)
add_test(NAME test_athrd_fallback COMMAND $<TARGET_FILE:test_athrd_fallback>)
list(APPEND ALIB_TEST_TARGETS test_athrd_fallback)

if(ALIB_TEST_TARGETS)
    add_custom_target(tests DEPENDS ${ALIB_TEST_TARGETS})
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
        DEPENDS ${ALIB_TEST_TARGETS}
        USES_TERMINAL
    )
endif()
