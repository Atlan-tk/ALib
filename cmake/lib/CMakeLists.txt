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
