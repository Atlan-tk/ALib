# Native Windows build toolchain that pins ALib to clang-cl.
# Use this from a Developer Command Prompt or an environment that already
# provides the Windows SDK, linker, and runtime libraries.

set(CMAKE_C_COMPILER clang-cl CACHE STRING
    "C compiler used for native Windows clang-cl builds"
)
