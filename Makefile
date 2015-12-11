.PHONY: all clean

PROF = false
DEBUG = false
OLEVEL = 2
GCC = gcc

CFLAG = 
ifeq ($(DEBUG), true)
	CFLAG += -O0 -g
else
	CFLAG += -O$(OLEVEL)
endif
ifeq ($(PROF), true)
	CFLAG += -pg
endif

BIN = lc3as
SRC = main.c lc3as.c
OBJ = $(SRC:.c=.o)

all:$(BIN)

%.o: %.c
	$(GCC) $< $(CFLAG) -c -o $@

$(BIN):$(OBJ)
	$(GCC) $(CFLAG) $^ -o $(BIN)

clean:
	-rm $(OBJ) $(BIN)
