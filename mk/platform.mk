# Shared platform detection for thread-related compiler and linker flags.
# Prefer the compiler target triple so cross-compiles pick the correct backend.

ALIB_TARGET_TRIPLE ?= $(shell $(CC) -dumpmachine 2>/dev/null)
ALIB_UNAME_S ?= $(shell uname -s 2>/dev/null || echo unknown)

ifneq (,$(filter %cygwin%,$(ALIB_TARGET_TRIPLE)))
ALIB_PLATFORM := posix
else ifneq (,$(filter %mingw% %msys% %windows%,$(ALIB_TARGET_TRIPLE)))
ALIB_PLATFORM := windows
else ifneq (,$(filter CYGWIN%,$(ALIB_UNAME_S)))
ALIB_PLATFORM := posix
else ifneq (,$(filter MINGW% MSYS%,$(ALIB_UNAME_S)))
ALIB_PLATFORM := windows
else ifeq ($(OS),Windows_NT)
ALIB_PLATFORM := windows
else
ALIB_PLATFORM := posix
endif

THREAD_CFLAGS ?=
THREAD_LIBS ?=
ALIB_PLATFORM_CFLAGS ?=

ifeq ($(ALIB_PLATFORM),posix)
ALIB_PLATFORM_CFLAGS += -D__C_POSIX__
ifeq ($(strip $(THREAD_CFLAGS)),)
THREAD_CFLAGS := -pthread
endif
ifeq ($(strip $(THREAD_LIBS)),)
THREAD_LIBS := -pthread
endif
else ifeq ($(ALIB_PLATFORM),windows)
ALIB_PLATFORM_CFLAGS += -D__C_WINDOWS__
endif
