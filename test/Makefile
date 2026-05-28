
SRC = $(wildcard $(DRI)*.c)
INC = $(wildcard $(ALIB)inc/*.h)
OBJ = $(patsubst $(DRI)%.c, $(BUILD)%.o, $(SRC))
TAR = $(patsubst $(BUILD)%.o, $(OUTDIR)%.$(EXE), $(OBJ))

.PHONY: all clean

all: $(TAR)

clean:
	rm -rf $(TAR) $(OBJ)

$(TAR): $(OUTDIR)%.$(EXE): $(BUILD)%.o
	$(CC) $< -o $@ $(CFLAGS_LD)

$(OBJ): $(BUILD)%.o: $(DRI)%.c $(INC)
	$(CC) $(CFLAGS_TEST) -c $< -o $@

