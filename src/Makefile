
SRC = $(wildcard $(DRI)*.c)
OBJ = $(patsubst $(DRI)%.c, $(BUILD)%.o, $(SRC))
TAR = $(TARLIB)


.PHONY: all clean

all: $(TAR)

clean:
	@rm -rf $(TAR) $(OBJ)

$(TAR): $(OBJ)
	$(AR) $(CFLAGS_AR) $@ $^

$(OBJ): $(BUILD)%.o: $(DRI)%.c
	$(CC) $(CFLAGS_CC) -c $< -o $@




