
CONFIG := mingw64

CC := x86_64-w64-mingw32-gcc
AR := x86_64-w64-mingw32ucrt-ar
export CC AR

EXE := exe
LIB := alib.lib
export EXE LIB

BUILD := $(ALIB)build/
OUTDIR := $(BUILD)out/
TARLIB := $(OUTDIR)$(LIB)
INCLUDE := $(BUILD).include/
export TARLIB OUTDIR BUILD INCLUDE

CFLAGS_AR := rcs
CFLAGS_LD := -lpthread -L$(OUTDIR) -lalib
CFLAGS_CC := -O3 -Wall -Wextra -Werror -fPIC -I$(ALIB)inc -DGNU_SOURCE
CFLAGS_TEST := -g -O0 -Wall -Wextra -Werror -I$(INCLUDE) -DGNU_SOURCE
export CFLAGS_CC CFLAGS_AR CFLAGS_LD CFLAGS_TEST

INSTALL_INC := $(HOME)/.alib/include/alib/
INSTALL_LIB := $(HOME)/.alib/lib/
export INSTALL_LIB INSTALL_INC




