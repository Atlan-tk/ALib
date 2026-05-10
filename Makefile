CC ?= gcc
AR ?= ar

PREFIX ?= /usr/local
DESTDIR ?=
INCLUDEDIR ?= $(PREFIX)/include/alib
LIBDIR ?= $(PREFIX)/lib

SRC_DIR := src
INC_DIR := inc
TEST_DIR := test
SAMPLE_DIR := sample

CPPFLAGS := -I$(INC_DIR)
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -O2 -fPIC
DEPFLAGS := -MMD -MP
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
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(DEPS) $(LIB_NAME)

install: $(LIB_NAME)
	mkdir -p $(DESTDIR)/$(INCLUDEDIR) $(DESTDIR)/$(LIBDIR)
	cp $(HEADERS) $(DESTDIR)/$(INCLUDEDIR)/
	cp $(LIB_NAME) $(DESTDIR)/$(LIBDIR)/

uninstall:
	rm -rf $(DESTDIR)/$(INCLUDEDIR)
	rm -rf $(DESTDIR)/$(LIBDIR)/$(LIB_NAME)

.PHONY: all clean install uninstall

-include $(DEPS)
