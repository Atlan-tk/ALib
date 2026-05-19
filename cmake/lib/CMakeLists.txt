add_library(atlan STATIC ${ALIB_PUBLIC_HEADERS} ${ALIB_SOURCES})

set(ALIB_STATIC_LIBRARY_PREFIX "lib")
set(ALIB_STATIC_LIBRARY_SUFFIX ".a")
if(WIN32)
    set(ALIB_STATIC_LIBRARY_PREFIX "")
    set(ALIB_STATIC_LIBRARY_SUFFIX ".lib")
endif()

set_target_properties(atlan PROPERTIES
    OUTPUT_NAME atlan
    EXPORT_NAME atlan
    POSITION_INDEPENDENT_CODE ON
    PREFIX "${ALIB_STATIC_LIBRARY_PREFIX}"
    SUFFIX "${ALIB_STATIC_LIBRARY_SUFFIX}"
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

if(WIN32 OR CYGWIN)
    set(ALIB_WINDOWS_UNSUPPORTED_SCRIPT
        "${PROJECT_BINARY_DIR}/cmake/alib_windows_unsupported.cmake"
    )
    file(GENERATE OUTPUT "${ALIB_WINDOWS_UNSUPPORTED_SCRIPT}" CONTENT
        "message(FATAL_ERROR \"ALib currently does not support Windows environments; configuration is kept for compatibility, but compilation is intentionally blocked.\")\n"
    )
    add_custom_target(alib_windows_unsupported_build
        COMMAND ${CMAKE_COMMAND} -P "${ALIB_WINDOWS_UNSUPPORTED_SCRIPT}"
        COMMENT "Stopping unsupported Windows build"
        VERBATIM
    )
    add_dependencies(atlan alib_windows_unsupported_build)
endif()
