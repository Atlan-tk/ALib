set(ALIB_CONFIG_DESCRIPTION "Windows native clang-cl build with Visual Studio project files")
set(ALIB_CONFIG_GENERATOR_REGEX "^Visual Studio")
set(ALIB_DISABLE_INSTALL ON)

if(NOT CMAKE_HOST_WIN32)
    message(FATAL_ERROR "CONFIG='windows-clang-cl-vs' must be configured on Windows.")
endif()

if(NOT CMAKE_GENERATOR_TOOLSET MATCHES "(^|,)ClangCL(,|$)")
    message(FATAL_ERROR
        "CONFIG='windows-clang-cl-vs' requires the Visual Studio ClangCL toolset. "
        "Reconfigure with: cmake -S . -B build -G \"Visual Studio 17 2022\" -T ClangCL -DCONFIG=windows-clang-cl-vs"
    )
endif()
