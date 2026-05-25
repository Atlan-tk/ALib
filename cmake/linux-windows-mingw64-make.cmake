set(ALIB_CONFIG_DESCRIPTION "Linux-to-Windows MinGW-w64 UCRT cross build with Makefiles")
set(ALIB_CONFIG_GENERATOR_REGEX "(^|.* )Makefiles$")
set(ALIB_INSTALL_KIND "linux-cross")

if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "CONFIG='linux-windows-mingw64-make' must be configured on Linux.")
endif()

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(ALIB_MINGW_TRIPLET "x86_64-w64-mingw32ucrt" CACHE STRING
    "UCRT MinGW-w64 target triplet used for Linux-to-Windows cross compilation"
)

find_program(ALIB_MINGW_GCC NAMES "${ALIB_MINGW_TRIPLET}-gcc")
if(NOT ALIB_MINGW_GCC)
    message(FATAL_ERROR
        "CONFIG='linux-windows-mingw64-make' requires a UCRT MinGW-w64 compiler. "
        "Could not find '${ALIB_MINGW_TRIPLET}-gcc'. Install a UCRT MinGW-w64/llvm-mingw toolchain "
        "or set ALIB_MINGW_TRIPLET to a UCRT triplet that provides <triplet>-gcc. "
        "Do not use an MSVCRT compiler such as 'x86_64-w64-mingw32-gcc' with manual ucrtbase linking."
    )
endif()

set(CMAKE_C_COMPILER "${ALIB_MINGW_GCC}" CACHE FILEPATH "UCRT MinGW-w64 C compiler" FORCE)
set(CMAKE_RC_COMPILER "${ALIB_MINGW_TRIPLET}-windres")
set(CMAKE_AR "${ALIB_MINGW_TRIPLET}-ar")
set(CMAKE_RANLIB "${ALIB_MINGW_TRIPLET}-ranlib")
set(CMAKE_NM "${ALIB_MINGW_TRIPLET}-nm")
set(CMAKE_STRIP "${ALIB_MINGW_TRIPLET}-strip")

set(ALIB_MINGW_ROOT "/usr/${ALIB_MINGW_TRIPLET}" CACHE PATH
    "Optional MinGW-w64 sysroot path"
)
if(EXISTS "${ALIB_MINGW_ROOT}")
    list(APPEND CMAKE_FIND_ROOT_PATH "${ALIB_MINGW_ROOT}")
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

function(alib_configure_platform)
    cmake_push_check_state(RESET)
    check_c_source_compiles([=[
        #include <time.h>

        int main(void) {
            struct timespec now;
            return timespec_get(&now, TIME_UTC) == TIME_UTC ? 0 : 1;
        }
    ]=] ALIB_MINGW_HAS_TIMESPEC_GET)
    cmake_pop_check_state()

    if(NOT ALIB_MINGW_HAS_TIMESPEC_GET)
        message(FATAL_ERROR
            "ALib MinGW builds require a UCRT-compatible compiler/runtime that provides "
            "timespec_get/TIME_UTC by default. Use a UCRT MinGW-w64 or llvm-mingw toolchain."
        )
    endif()
endfunction()
