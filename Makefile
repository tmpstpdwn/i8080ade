CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -Isrc
LDFLAGS :=

SRC     := $(shell find src -name '*.c')
OBJ     := $(SRC:src/%.c=build/%.o)
DEP     := $(OBJ:.o=.d)

TARGET := build/i8080ade

TEST_SRC := tests/test_encode.c tests/test_decode.c
TEST_BIN := $(TEST_SRC:tests/%.c=build/tests/%)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $^ -o $@

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

build/tests/%: tests/%.c build/i8080/i8080.o build/util/slice.o
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

test: $(TEST_BIN)
	@for t in $(TEST_BIN); do \
		echo "Running $$t..."; \
		$$t || exit 1; \
	done

debug: CFLAGS += -g -O0 -fsanitize=address,undefined,leak -fanalyzer
debug: LDFLAGS += -fsanitize=address,undefined,leak
debug: clean all

clean:
	rm -rf build

-include $(DEP)

.PHONY: all debug test clean
