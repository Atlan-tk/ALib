cmake_minimum_required(VERSION 3.16)

set(ALIB_TOP_LEVEL_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ALIB_TOP_LEVEL_PROJECT ON)
endif()

set(CONFIG "" CACHE STRING "ALib build configuration name")

if(ALIB_TOP_LEVEL_PROJECT)
    function(alib_normalize_config input output)
        string(TOLOWER "${input}" ALIB_CONFIG_LOWER)
        string(REPLACE "_" "-" ALIB_CONFIG_LOWER "${ALIB_CONFIG_LOWER}")
        set(${output} "${ALIB_CONFIG_LOWER}" PARENT_SCOPE)
    endfunction()

    file(GLOB ALIB_CONFIG_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake")
    set(ALIB_AVAILABLE_CONFIGS "")
    foreach(ALIB_CONFIG_CANDIDATE IN LISTS ALIB_CONFIG_FILES)
        get_filename_component(ALIB_CONFIG_NAME "${ALIB_CONFIG_CANDIDATE}" NAME_WE)
        list(APPEND ALIB_AVAILABLE_CONFIGS "${ALIB_CONFIG_NAME}")
    endforeach()
    list(SORT ALIB_AVAILABLE_CONFIGS)
    set_property(CACHE CONFIG PROPERTY STRINGS ${ALIB_AVAILABLE_CONFIGS})

    if(NOT CONFIG)
        if(CMAKE_HOST_WIN32 AND "windows-clang-cl-vs" IN_LIST ALIB_AVAILABLE_CONFIGS)
            set(CONFIG "windows-clang-cl-vs" CACHE STRING "ALib build configuration name" FORCE)
        elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND "linux-gcc-make" IN_LIST ALIB_AVAILABLE_CONFIGS)
            set(CONFIG "linux-gcc-make" CACHE STRING "ALib build configuration name" FORCE)
        elseif(UNIX AND "unix-gcc-make" IN_LIST ALIB_AVAILABLE_CONFIGS)
            set(CONFIG "unix-gcc-make" CACHE STRING "ALib build configuration name" FORCE)
        else()
            message(FATAL_ERROR "Cannot auto-select ALib CONFIG for host '${CMAKE_HOST_SYSTEM_NAME}'.")
        endif()
    endif()

    alib_normalize_config("${CONFIG}" ALIB_SELECTED_CONFIG)
    set(CONFIG "${ALIB_SELECTED_CONFIG}" CACHE STRING "ALib build configuration name" FORCE)

    set(ALIB_CONFIG_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${CONFIG}.cmake")
    if(NOT EXISTS "${ALIB_CONFIG_FILE}")
        string(REPLACE ";" ", " ALIB_AVAILABLE_CONFIG_TEXT "${ALIB_AVAILABLE_CONFIGS}")
        message(FATAL_ERROR
            "Unknown ALib CONFIG='${CONFIG}'. Add '${CONFIG}.cmake' under '${CMAKE_CURRENT_SOURCE_DIR}/cmake' "
            "or choose one of: ${ALIB_AVAILABLE_CONFIG_TEXT}."
        )
    endif()

    include("${ALIB_CONFIG_FILE}")

    if(DEFINED ALIB_CONFIG_GENERATOR_REGEX AND NOT CMAKE_GENERATOR MATCHES "${ALIB_CONFIG_GENERATOR_REGEX}")
        message(FATAL_ERROR
            "CONFIG='${CONFIG}' requires a CMake generator matching "
            "'${ALIB_CONFIG_GENERATOR_REGEX}', but current generator is '${CMAKE_GENERATOR}'. "
            "Reconfigure ALib/build with the required -G option."
        )
    endif()
endif()

project(ALib LANGUAGES C)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang|IntelLLVM")
    string(REPLACE "-O3" "-O0" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING
        "Flags used by the C compiler during Release builds." FORCE)
endif()

if(ALIB_TOP_LEVEL_PROJECT AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(ALIB_INSTALL_KIND STREQUAL "linux-cross")
        if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
            set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.alib" CACHE PATH "Install path prefix." FORCE)
        else()
            message(FATAL_ERROR "HOME is required to select the default Linux cross install prefix.")
        endif()
    elseif(NOT ALIB_DISABLE_INSTALL)
        set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install path prefix." FORCE)
    endif()
endif()

include(GNUInstallDirs)

set(ALIB_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
set(ALIB_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/alib")
if(ALIB_INSTALL_KIND STREQUAL "linux-cross")
    set(ALIB_INSTALL_LIBDIR "lib")
    set(ALIB_INSTALL_INCLUDEDIR "inc/alib")
endif()
include(CMakePushCheckState)
include(CheckCSourceCompiles)

if(NOT (WIN32 OR UNIX))
    message(FATAL_ERROR
        "ALib cannot be built for platform '${CMAKE_SYSTEM_NAME}'. "
        "Only Linux/Unix/Windows targets are supported."
    )
endif()

if(ALIB_TOP_LEVEL_PROJECT AND NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ALIB_MSVC_LIKE OFF)
if(MSVC OR CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(ALIB_MSVC_LIKE ON)
endif()

if(ALIB_TOP_LEVEL_PROJECT)
    message(STATUS "ALib CONFIG: ${CONFIG}")
    if(DEFINED ALIB_CONFIG_DESCRIPTION)
        message(STATUS "ALib config description: ${ALIB_CONFIG_DESCRIPTION}")
    endif()
endif()

set(ALIB_REQUIRED_C_STANDARD_FLAG "")
if(ALIB_MSVC_LIKE)
    set(ALIB_REQUIRED_C_STANDARD_FLAG "/std:c11")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang|IntelLLVM")
    set(ALIB_REQUIRED_C_STANDARD_FLAG "-std=c11")
endif()

cmake_push_check_state(RESET)
if(ALIB_REQUIRED_C_STANDARD_FLAG)
    set(CMAKE_REQUIRED_FLAGS "${ALIB_REQUIRED_C_STANDARD_FLAG}")
endif()
check_c_source_compiles([=[
    int main(void) {
        int value = 1;
        _Static_assert(sizeof(int) >= 2, "ALib requires C11 _Static_assert support");
        return _Generic((value), int: 0, default: 1);
    }
]=] ALIB_COMPILER_SUPPORTS_C11)
cmake_pop_check_state()

if(NOT ALIB_COMPILER_SUPPORTS_C11)
    message(FATAL_ERROR
        "ALib cannot be built with compiler '${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}'. "
        "A C11-capable compiler is required."
    )
endif()

cmake_push_check_state(RESET)
if(ALIB_REQUIRED_C_STANDARD_FLAG)
    set(CMAKE_REQUIRED_FLAGS "${ALIB_REQUIRED_C_STANDARD_FLAG}")
endif()
check_c_source_compiles([=[
    static void cleanup_int(int *value) { (void)value; }
    static void base_symbol(void) {}
    static void weak_alias(void) __attribute__((weakref("base_symbol")));
    __attribute__((constructor)) static void ctor_symbol(void) {}

    int main(void) {
        __auto_type x = 1;
        typeof(x) y = x;
        __attribute__((cleanup(cleanup_int))) int z = 0;
        (void)weak_alias;
        (void)ctor_symbol;
        return y + z;
    }
]=] ALIB_COMPILER_SUPPORTS_GNU_EXTENSIONS)
cmake_pop_check_state()

if(NOT ALIB_COMPILER_SUPPORTS_GNU_EXTENSIONS)
    message(FATAL_ERROR
        "ALib cannot be built with compiler '${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}'. "
        "GNU C extensions are required, including __auto_type, typeof, cleanup, and weakref."
    )
endif()

option(ALIB_BUILD_SAMPLES "Build sample programs" ON)
option(ALIB_BUILD_TESTS "Build regression tests" ON)

file(GLOB ALIB_PUBLIC_HEADERS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/inc/*.h")
file(GLOB ALIB_SOURCES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/src/*.c")

set(ALIB_OBJECT_ROOT "${PROJECT_BINARY_DIR}/obj")
set(ALIB_LIBRARY_OUTPUT_DIR "${PROJECT_BINARY_DIR}/lib")
set(ALIB_SAMPLE_OUTPUT_DIR "${PROJECT_BINARY_DIR}/sample")
set(ALIB_TEST_OUTPUT_DIR "${PROJECT_BINARY_DIR}/test")
set(ALIB_BUILD_INCLUDE_ROOT "${PROJECT_BINARY_DIR}/inc")
set(ALIB_BUILD_ALIB_INCLUDE_DIR "${ALIB_BUILD_INCLUDE_ROOT}/alib")

foreach(ALIB_LEGACY_INCLUDE_LINK IN ITEMS
        "${PROJECT_BINARY_DIR}/inc"
        "${PROJECT_BINARY_DIR}/alib")
    if(IS_SYMLINK "${ALIB_LEGACY_INCLUDE_LINK}")
        file(REMOVE "${ALIB_LEGACY_INCLUDE_LINK}")
    endif()
endforeach()

file(MAKE_DIRECTORY
    "${ALIB_OBJECT_ROOT}"
    "${ALIB_LIBRARY_OUTPUT_DIR}"
    "${ALIB_SAMPLE_OUTPUT_DIR}"
    "${ALIB_TEST_OUTPUT_DIR}"
    "${ALIB_BUILD_INCLUDE_ROOT}"
    "${ALIB_BUILD_ALIB_INCLUDE_DIR}"
)

foreach(ALIB_HEADER IN LISTS ALIB_PUBLIC_HEADERS)
    get_filename_component(ALIB_HEADER_NAME "${ALIB_HEADER}" NAME)
    configure_file(
        "${ALIB_HEADER}"
        "${ALIB_BUILD_ALIB_INCLUDE_DIR}/${ALIB_HEADER_NAME}"
        COPYONLY
    )
endforeach()

function(alib_enable_warnings target)
    if(ALIB_MSVC_LIKE)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    endif()
endfunction()

function(alib_apply_defaults target)
    target_compile_features(${target} PRIVATE c_std_11)
    alib_enable_warnings(${target})
endfunction()

function(alib_apply_dev_flags target)
    if(ALIB_MSVC_LIKE)
        target_compile_options(${target} PRIVATE /Od /Zi /UNDEBUG)
    else()
        target_compile_options(${target} PRIVATE -O0 -g -UNDEBUG)
    endif()
endfunction()

function(alib_apply_platform target)
    if(NOT (CYGWIN OR WIN32 OR UNIX))
        message(FATAL_ERROR "ALib cannot be built for unsupported platform '${CMAKE_SYSTEM_NAME}'.")
    endif()
endfunction()

find_package(Threads REQUIRED)

set(ALIB_PUBLIC_COMPILE_DEFINITIONS "")
set(ALIB_PUBLIC_LINK_LIBRARIES Threads::Threads)

if(COMMAND alib_configure_platform)
    alib_configure_platform()
endif()

add_subdirectory(src "${ALIB_OBJECT_ROOT}/lib")
add_library(ALib::atlan ALIAS atlan)

if(ALIB_BUILD_SAMPLES)
    add_subdirectory(sample "${ALIB_OBJECT_ROOT}/sample")
endif()

if(ALIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test "${ALIB_OBJECT_ROOT}/test")
endif()

if(NOT ALIB_DISABLE_INSTALL)
    install(TARGETS atlan
        ARCHIVE DESTINATION "${ALIB_INSTALL_LIBDIR}"
    )
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/inc/"
        DESTINATION "${ALIB_INSTALL_INCLUDEDIR}"
        FILES_MATCHING PATTERN "*.h"
    )
endif()
