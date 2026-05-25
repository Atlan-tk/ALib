# ALib CMake configurations

Configure from the repository root and keep generated files in `build`:

```sh
cmake --fresh -S . -B build -G "Unix Makefiles" -DCONFIG=linux-gcc-make
cmake --build build
ctest --test-dir build --output-on-failure
```

Bundled `CONFIG` values:

- `linux-gcc-make`: Linux/Unix native GCC build, Makefiles.
- `unix-gcc-make`: Unix native GCC build alias, Makefiles.
- `linux-clang-make`: Linux/Unix native Clang build with GCC-style command-line flags, Makefiles.
- `unix-clang-make`: Unix native Clang build alias with GCC-style command-line flags, Makefiles.
- `windows-clang-cl-vs`: Windows native `clang-cl`, Visual Studio project files; use `-T ClangCL`.
- `linux-windows-mingw64-make`: Linux-to-Windows UCRT MinGW-w64 cross build, Makefiles.
- `linux-windows-clang-cl-make`: Linux-to-Windows `clang-cl` cross build, Makefiles.
- `linux-linux-gcc-cross-make`: Linux-to-Linux GCC cross build, Makefiles; set `ALIB_LINUX_GCC_TRIPLET`.

The top-level `CMakeLists.txt` scans this directory; add a new `<name>.cmake` file directly under `cmake` to add a new `CONFIG=<name>` option. If `CONFIG` is not set, CMake selects `windows-clang-cl-vs` on Windows, `linux-gcc-make` on Linux, and `unix-gcc-make` on other Unix hosts.
CMake generators and compiler settings are cache-bound, so use `cmake --fresh` or a clean `build` directory when switching `CONFIG` values.

## Install layout

Use `cmake --build build --target install` or `make install` from `build`.

- Linux/Unix native configs install headers to `/usr/local/include/alib` and the static library to `/usr/local/lib`.
- Linux cross configs install headers to `$HOME/.alib/inc/alib` and the static library to `$HOME/.alib/lib`.
- Windows native config does not create install rules.
