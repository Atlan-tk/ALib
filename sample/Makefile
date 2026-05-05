
CC := gcc

WORK := ..

CFLAGS := -std=c11 -Wall -Wextra -Werror -g -O0 -I.

SRC :=             	\
    sample_type.c   \
    sample_aline.c  \
    sample_aclass.c \

HEAD :=				\

OUT := $(SRC:.c=.out)

LIB := atlan

all: $(OUT)

%.out: %.c $(HEAD)
	$(CC) $(CFLAGS) $< -l$(LIB) -o $@

# Clean target
clean:
	rm -rf $(OUT)

# Phony targets
.PHONY: all clean


