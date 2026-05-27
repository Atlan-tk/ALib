
CONFIG := linux-mips

CC := mips-linux-uclibc-gnu-gcc
AR := mips-linux-uclibc-gnu-ar
export CC AR

EXE := out
LIB := libalib.a
export EXE LIB

BUILD := $(ALIB)build/
OUTDIR := $(BUILD)out/
TARLIB := $(OUTDIR)$(LIB)
INCLUDE := $(BUILD).include/
export TARLIB OUTDIR BUILD INCLUDE

CFLAGS_AR := rcs
CFLAGS_LD := -lpthread -lgcc_s -L$(OUTDIR) -lalib
CFLAGS_CC := -O2 -Wall -Wextra -Werror -fPIC -I$(ALIB)inc -DGNU_SOURCE
CFLAGS_TEST := -g -O0 -Wall -Wextra -Werror -I$(INCLUDE) -DGNU_SOURCE
export CFLAGS_CC CFLAGS_AR CFLAGS_LD CFLAGS_TEST

INSTALL_INC := $(HOME)/.alib/include/alib/
INSTALL_LIB := $(HOME)/.alib/lib/
export INSTALL_LIB INSTALL_INC



