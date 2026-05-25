set(ALIB_CONFIG_DESCRIPTION "Linux-to-Windows clang-cl cross build with Makefiles")
set(ALIB_CONFIG_GENERATOR_REGEX "(^|.* )Makefiles$")
set(ALIB_INSTALL_KIND "linux-cross")

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "CONFIG='linux-windows-clang-cl-make' must be configured on Linux.")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(ALIB_CLANG_CL_TARGET "x86_64-pc-windows-msvc" CACHE STRING
    "clang-cl target triple used for Linux-to-Windows cross compilation"
)
set(ALIB_WINDOWS_SYSROOT "" CACHE PATH
    "Optional Windows SDK/MSVC-compatible sysroot for clang-cl cross compilation"
)

set(CMAKE_C_COMPILER clang-cl CACHE STRING "Windows cross C compiler")
set(CMAKE_C_COMPILER_TARGET "${ALIB_CLANG_CL_TARGET}")
set(CMAKE_LINKER lld-link CACHE STRING "Windows cross linker")
set(CMAKE_AR llvm-lib CACHE STRING "Windows static library tool")
set(CMAKE_MT llvm-mt CACHE STRING "Windows manifest tool")

set(CMAKE_C_FLAGS_INIT "/clang:-fms-compatibility-version=19")
if(ALIB_WINDOWS_SYSROOT)
    set(CMAKE_SYSROOT "${ALIB_WINDOWS_SYSROOT}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
