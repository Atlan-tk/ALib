CC ?= gcc
AR ?= ar

include mk/platform.mk

DESTDIR ?=
PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include/alib

DESTROOT := $(patsubst %/,%,$(DESTDIR))
INSTALL_LIBDIR := $(if $(DESTROOT),$(DESTROOT),)$(LIBDIR)
INSTALL_INCLUDEDIR := $(if $(DESTROOT),$(DESTROOT),)$(INCLUDEDIR)

SRC_DIR := src
INC_DIR := inc
TEST_DIR := test
SAMPLE_DIR := sample

CFLAGS := $(ALIB_PLATFORM_CFLAGS) $(THREAD_CFLAGS) -I$(INC_DIR) -Wall -Wextra -Werror -O2 -fPIC -MMD -MP
ARFLAGS ?= rcs

LIB_NAME := libatlan.a
SRCS := $(sort $(wildcard $(SRC_DIR)/*.c))
OBJS := $(SRCS:.c=.o)
DEPS := $(OBJS:.o=.d)
HEADERS := $(sort $(wildcard $(INC_DIR)/*.h))

TEST_BINS := $(sort $(patsubst %.c,%.out,$(wildcard $(TEST_DIR)/test_*.c)))
SAMPLE_BINS := $(sort $(patsubst %.c,%.out,$(wildcard $(SAMPLE_DIR)/*.c)))

.DEFAULT_GOAL := all
.DELETE_ON_ERROR:

all: $(LIB_NAME)

$(LIB_NAME): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEPS) $(LIB_NAME)
	$(MAKE) clean -C sample
	$(MAKE) clean -C test

install: $(LIB_NAME)
	mkdir -p $(INSTALL_INCLUDEDIR) $(INSTALL_LIBDIR)
	cp $(HEADERS) $(INSTALL_INCLUDEDIR)/
	cp $(LIB_NAME) $(INSTALL_LIBDIR)/

uninstall:
	rm -rf $(INSTALL_INCLUDEDIR)
	rm -rf $(INSTALL_LIBDIR)/$(LIB_NAME)

.PHONY: all clean install uninstall

-include $(DEPS)
