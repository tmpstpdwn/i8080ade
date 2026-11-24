
CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc
LDFLAGS :=

SRC     := $(shell find src -name '*.c')
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

TARGET := build/i8080ade

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

debug: CFLAGS += -g -O0 -fsanitize=address,undefined,leak -fanalyzer
debug: LDFLAGS += -fsanitize=address,undefined,leak
debug: clean all

clean:
	rm -rf build

-include $(DEP)

.PHONY: all debug test clean
