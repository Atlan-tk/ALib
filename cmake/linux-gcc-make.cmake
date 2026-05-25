set(ALIB_CONFIG_DESCRIPTION "Linux/Unix native GCC build with Makefiles")
set(ALIB_CONFIG_GENERATOR_REGEX "(^|.* )Makefiles$")

if(CMAKE_HOST_WIN32)
    message(FATAL_ERROR "CONFIG='linux-gcc-make' must be configured on a Unix-like host.")
endif()

if(NOT DEFINED CMAKE_C_COMPILER AND (NOT DEFINED ENV{CC} OR "$ENV{CC}" STREQUAL ""))
    find_program(ALIB_CONFIG_GCC NAMES gcc REQUIRED)
    set(CMAKE_C_COMPILER "${ALIB_CONFIG_GCC}" CACHE FILEPATH "C compiler for ${CONFIG}" FORCE)
endif()

set(ALIB_INSTALL_KIND "native-unix")
