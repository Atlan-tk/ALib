add_library(atlan STATIC ${ALIB_PUBLIC_HEADERS} ${ALIB_SOURCES})

set_target_properties(atlan PROPERTIES
    OUTPUT_NAME atlan
    EXPORT_NAME atlan
    POSITION_INDEPENDENT_CODE ON
    ARCHIVE_OUTPUT_DIRECTORY "${ALIB_LIBRARY_OUTPUT_DIR}"
)

target_include_directories(atlan
    PRIVATE "${PROJECT_SOURCE_DIR}/inc"
    PUBLIC
        $<BUILD_INTERFACE:${ALIB_BUILD_INCLUDE_ROOT}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(atlan PUBLIC Threads::Threads)
alib_apply_defaults(atlan)
alib_apply_platform(atlan)
