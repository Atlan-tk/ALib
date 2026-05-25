set(ALIB_CONFIG_DESCRIPTION "Linux/Unix native Clang build with GCC-style flags and Makefiles")
set(ALIB_CONFIG_GENERATOR_REGEX "(^|.* )Makefiles$")

if(CMAKE_HOST_WIN32)
    message(FATAL_ERROR "CONFIG='linux-clang-make' must be configured on a Unix-like host.")
endif()

if(NOT DEFINED CMAKE_C_COMPILER AND (NOT DEFINED ENV{CC} OR "$ENV{CC}" STREQUAL ""))
    find_program(ALIB_CONFIG_CLANG NAMES clang REQUIRED)
    set(CMAKE_C_COMPILER "${ALIB_CONFIG_CLANG}" CACHE FILEPATH "C compiler for ${CONFIG}" FORCE)
endif()

set(ALIB_INSTALL_KIND "native-unix")
