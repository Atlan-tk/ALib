cmake_minimum_required(VERSION 3.16)

set(ALIB_TOP_LEVEL_PROJECT OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ALIB_TOP_LEVEL_PROJECT ON)
endif()

if(ALIB_TOP_LEVEL_PROJECT
        AND NOT DEFINED CMAKE_TOOLCHAIN_FILE
        AND (NOT DEFINED ENV{CC} OR "$ENV{CC}" STREQUAL "")
        AND NOT DEFINED CMAKE_C_COMPILER)
    if(CMAKE_HOST_WIN32)
        if(CMAKE_GENERATOR MATCHES "^Visual Studio" AND NOT DEFINED CMAKE_GENERATOR_TOOLSET)
            set(CMAKE_GENERATOR_TOOLSET ClangCL CACHE STRING "Default toolset for ALib" FORCE)
            set(ALIB_DEFAULT_COMPILER_HINT "ClangCL")
        else()
            find_program(ALIB_BOOTSTRAP_CLANG_CL NAMES clang-cl)
            if(ALIB_BOOTSTRAP_CLANG_CL)
                set(CMAKE_C_COMPILER "${ALIB_BOOTSTRAP_CLANG_CL}" CACHE FILEPATH "Default C compiler for ALib" FORCE)
                set(ALIB_DEFAULT_COMPILER_HINT "${ALIB_BOOTSTRAP_CLANG_CL}")
            endif()
        endif()
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
        find_program(ALIB_BOOTSTRAP_GCC NAMES gcc)
        if(ALIB_BOOTSTRAP_GCC)
            set(CMAKE_C_COMPILER "${ALIB_BOOTSTRAP_GCC}" CACHE FILEPATH "Default C compiler for ALib" FORCE)
            set(ALIB_DEFAULT_COMPILER_HINT "${ALIB_BOOTSTRAP_GCC}")
        endif()
    endif()
endif()

project(ALib LANGUAGES C)

if(ALIB_TOP_LEVEL_PROJECT AND CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(WIN32)
        set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/alib" CACHE PATH "Install path prefix." FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Install path prefix." FORCE)
    endif()
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
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

if(ALIB_TOP_LEVEL_PROJECT AND DEFINED ALIB_DEFAULT_COMPILER_HINT)
    message(STATUS "ALib default C compiler/toolset: ${ALIB_DEFAULT_COMPILER_HINT}")
endif()

set(ALIB_REQUIRED_C_STANDARD_FLAG "")
if(ALIB_MSVC_LIKE)
    set(ALIB_REQUIRED_C_STANDARD_FLAG "/std:c11")
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang|AppleClang|IntelLLVM")
    set(ALIB_REQUIRED_C_STANDARD_FLAG "-std=gnu11")
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
set(ALIB_BUILD_INCLUDE_ROOT "${PROJECT_BINARY_DIR}")

function(alib_create_link link_path target_path)
    if(NOT EXISTS "${link_path}")
        file(CREATE_LINK
            "${target_path}"
            "${link_path}"
            SYMBOLIC
            COPY_ON_ERROR
            RESULT ALIB_LINK_RESULT
        )
        if(ALIB_LINK_RESULT)
            message(FATAL_ERROR "failed to create link '${link_path}': ${ALIB_LINK_RESULT}")
        endif()
    endif()
endfunction()

file(MAKE_DIRECTORY
    "${ALIB_OBJECT_ROOT}"
    "${ALIB_LIBRARY_OUTPUT_DIR}"
    "${ALIB_SAMPLE_OUTPUT_DIR}"
    "${ALIB_TEST_OUTPUT_DIR}"
)
alib_create_link("${PROJECT_BINARY_DIR}/inc" "${PROJECT_SOURCE_DIR}/inc")
alib_create_link("${PROJECT_BINARY_DIR}/alib" "${PROJECT_SOURCE_DIR}/inc")

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
    if(CYGWIN)
        target_compile_definitions(${target} PUBLIC __C_POSIX__)
    elseif(WIN32)
        target_compile_definitions(${target} PUBLIC __C_WINDOWS__)
    elseif(UNIX)
        target_compile_definitions(${target} PUBLIC __C_POSIX__)
    else()
        message(FATAL_ERROR "ALib cannot be built for unsupported platform '${CMAKE_SYSTEM_NAME}'.")
    endif()
endfunction()

find_package(Threads REQUIRED)

add_subdirectory(cmake/lib "${ALIB_OBJECT_ROOT}/lib")
add_library(ALib::atlan ALIAS atlan)

if(ALIB_BUILD_SAMPLES)
    add_subdirectory(cmake/sample "${ALIB_OBJECT_ROOT}/sample")
endif()

if(ALIB_BUILD_TESTS)
    enable_testing()
    add_subdirectory(cmake/test "${ALIB_OBJECT_ROOT}/test")
endif()

install(TARGETS atlan EXPORT ALibTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/inc/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/alib"
    FILES_MATCHING PATTERN "*.h"
)

set(ALIB_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/ALib")
set(ALIB_PACKAGE_CONFIG_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
if(WIN32 AND NOT CMAKE_HOST_WIN32 AND NOT IS_ABSOLUTE "${ALIB_PACKAGE_CONFIG_INSTALL_PREFIX}")
    # Cross-compiling to Windows from a Unix host keeps the Windows-style
    # install prefix for installation, but CMake package generation needs a
    # host-absolute path to compute PACKAGE_PREFIX_DIR.
    set(ALIB_PACKAGE_CONFIG_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/_package_prefix")
endif()
configure_package_config_file(
    "${PROJECT_SOURCE_DIR}/cmake/ALibConfig.cmake.in"
    "${PROJECT_BINARY_DIR}/ALibConfig.cmake"
    INSTALL_DESTINATION "${ALIB_INSTALL_CMAKEDIR}"
    INSTALL_PREFIX "${ALIB_PACKAGE_CONFIG_INSTALL_PREFIX}"
)

install(FILES "${PROJECT_BINARY_DIR}/ALibConfig.cmake"
    DESTINATION "${ALIB_INSTALL_CMAKEDIR}"
)
install(EXPORT ALibTargets
    FILE ALibTargets.cmake
    NAMESPACE ALib::
    DESTINATION "${ALIB_INSTALL_CMAKEDIR}"
)
