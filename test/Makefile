
CC ?= gcc

WORK ?= ..
ROOT_DIR := $(abspath $(WORK))
LOCAL_INCLUDE_DIR := $(ROOT_DIR)/.local/include
LOCAL_INCLUDE_LINK := $(LOCAL_INCLUDE_DIR)/alib
LIB_TARGET := $(ROOT_DIR)/libatlan.a

CPPFLAGS += -I$(LOCAL_INCLUDE_DIR)
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -g -O0

SRC := $(sort $(wildcard test_*.c))

HEAD :=				\

OUT := $(SRC:.c=.out)

all: $(OUT)

$(LIB_TARGET): FORCE
	$(MAKE) -C $(ROOT_DIR) $(notdir $@)

$(LOCAL_INCLUDE_LINK): FORCE
	mkdir -p $(LOCAL_INCLUDE_DIR)
	ln -snf $(ROOT_DIR)/inc $@

%.out: %.c $(HEAD) $(LIB_TARGET) | $(LOCAL_INCLUDE_LINK)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(LIB_TARGET)

# Clean target
clean:
	rm -rf $(OUT)

# Phony targets
.PHONY: all clean FORCE

FORCE:
