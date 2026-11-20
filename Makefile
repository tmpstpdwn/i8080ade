CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Isrc
SRC     := $(shell find src -name '*.c')
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

all: build/i8080

build/i8080: $(OBJ)
	$(CC) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

debug: CFLAGS += -g -O0 -fsanitize=address,undefined,leak -fanalyzer
debug: clean all

test: $(filter-out build/main.o,$(OBJ)) tests/test_opcodes.c
	$(CC) $(CFLAGS) $^ -o build/test_opcodes && ./build/test_opcodes

clean:
	rm -rf build

-include $(DEP)
.PHONY: all debug test clean
