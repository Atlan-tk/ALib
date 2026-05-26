
CC := cc
AR := ar
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
CFLAGS_LD := -lpthread -L$(OUTDIR) -lalib
CFLAGS_CC := -O3 -Wall -Werror -fPIC -I$(ALIB)inc
CFLAGS_TEST := -g -O0 -Wall -Werror -I$(INCLUDE)
export CFLAGS_CC CFLAGS_AR CFLAGS_LD CFLAGS_TEST

INSTALL_INC := /usr/local/include/alib/
INSTALL_LIB := /usr/local/lib/$(LIB)
export INSTALL_LIB INSTALL_INC




