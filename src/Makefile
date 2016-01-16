.PHONY: all clean

PROF = false
DEBUG = true
OLEVEL = 2
GCC = gcc

CFLAGS = -Wall -lm -lreadline
ifeq ($(DEBUG), true)
	CFLAGS += -O0 -g
else
	CFLAGS += -O$(OLEVEL)
endif
ifeq ($(PROF), true)
	CFLAGS += -pg
endif

BIN = simulator
SRC = main.c command.c rl.c label.c
OBJ = $(SRC:.c=.o)
BUILD_DIR = ../build
SRC_DIR = ./
BUILD = $(foreach n,$(OBJ),$(BUILD_DIR)/$(n))

all: $(BUILD_DIR) $(BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN): $(BUILD)
	$(GCC) $(CFLAGS) $^ -o $(BIN)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(GCC) $< $(CFLAGS) -c -o $@

clean:
	-rm $(BUILD) $(BIN)
