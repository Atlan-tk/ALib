file(GLOB ALIB_SAMPLE_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/sample/*.c")
set(ALIB_SAMPLE_TARGETS "")

foreach(sample_source IN LISTS ALIB_SAMPLE_SOURCES)
    get_filename_component(sample_target "${sample_source}" NAME_WE)
    add_executable(${sample_target} "${sample_source}")
    set_target_properties(${sample_target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${ALIB_SAMPLE_OUTPUT_DIR}"
    )
    target_link_libraries(${sample_target} PRIVATE atlan)
    alib_apply_defaults(${sample_target})
    alib_apply_dev_flags(${sample_target})
    list(APPEND ALIB_SAMPLE_TARGETS ${sample_target})
endforeach()

if(ALIB_SAMPLE_TARGETS)
    add_custom_target(samples DEPENDS ${ALIB_SAMPLE_TARGETS})
endif()
