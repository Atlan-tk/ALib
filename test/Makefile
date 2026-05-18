
CC ?= gcc

WORK := ..
ROOT_DIR := $(abspath $(WORK))
include $(ROOT_DIR)/mk/platform.mk

LIB_TARGET := $(ROOT_DIR)/libatlan.a
LOCAL_INCLUDE_DIR := $(ROOT_DIR)/.local/include
LOCAL_INCLUDE_LINK := $(LOCAL_INCLUDE_DIR)/alib

CFLAGS := $(ALIB_PLATFORM_CFLAGS) $(THREAD_CFLAGS) -I$(LOCAL_INCLUDE_DIR) -Wall -Wextra -Werror -g -O0

SRC := $(sort $(wildcard *.c))

OUT := $(SRC:.c=.out) test_athrd_fallback.out

all: $(OUT)

$(LIB_TARGET): FORCE
	$(MAKE) -C $(ROOT_DIR) $(notdir $@)

$(LOCAL_INCLUDE_LINK): FORCE
	mkdir -p $(LOCAL_INCLUDE_DIR)
	ln -snf $(ROOT_DIR)/inc $@

%.out: %.c $(LIB_TARGET) | $(LOCAL_INCLUDE_LINK)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_TARGET) $(THREAD_LIBS)

test_athrd.out: test_athrd.c $(LIB_TARGET) | $(LOCAL_INCLUDE_LINK)
	$(CC) $(CFLAGS) -o $@ $< $(LIB_TARGET) $(THREAD_LIBS)

test_athrd_fallback.out: test_athrd.c $(ROOT_DIR)/src/athrd.c | $(LOCAL_INCLUDE_LINK)
	$(CC) $(CFLAGS) -I$(ROOT_DIR)/inc -D__STDC_NO_THREADS__=1 -o $@ $^ $(THREAD_LIBS)

# Clean target
clean:
	rm -rf $(OUT)

# Phony targets
.PHONY: all clean FORCE

FORCE:
