#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#include "assembler.h"

typedef struct {
    char source[64];
    uint32_t expected;
} Test;

static Test tests[512];
static int count = 0;
static int passed = 0;
static int failed = 0;

static void add(uint32_t expected, const char *fmt, ...) {
    Test *t = &tests[count++];

    va_list args;
    va_start(args, fmt);
    vsnprintf(t->source, sizeof(t->source), fmt, args);
    va_end(args);

    t->expected = expected;
}

static void run(void) {
    for (int i = 0; i < count; i++) {
        uint32_t actual = assemble(tests[i].source, i);

        if (actual == tests[i].expected) {
            passed++;
        } else {
            failed++;
            printf("FAIL: %-24s EXPECTED: 0x%X GOT: 0x%X\n",
                   tests[i].source,
                   tests[i].expected,
                   actual);
        }
    }

    if (failed) printf("\n");

    printf("TOTAL:  %d\n", count);
    printf("PASSED: %d\n", passed);
    printf("FAILED: %d\n", failed);
}

// Register and Conditions def

static const char *regs[] = { "B", "C", "D", "E", "H", "L", "M", "A" };
static const char *pairs[] = { "B", "D", "H", "SP" };
static const char *stack_pairs[] = { "B", "D", "H", "PSW" };
static const char *conditions[] = { "NZ", "Z", "NC", "C", "PO", "PE", "P", "M" };

// Tests

static void test_single(void) {
    struct {
        const char *name;
        uint8_t opcode;
    } cases[] = {
        {"NOP",  0x00},

        {"RLC",  0x07},
        {"RRC",  0x0F},
        {"RAL",  0x17},
        {"RAR",  0x1F},

        {"DAA",  0x27},
        {"CMA",  0x2F},
        {"STC",  0x37},
        {"CMC",  0x3F},

        {"HLT",  0x76},

        {"XTHL", 0xE3},
        {"PCHL", 0xE9},
        {"XCHG", 0xEB},
        {"DI",   0xF3},
        {"SPHL", 0xF9},
        {"EI",   0xFB},
    };

    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++)
        add(cases[i].opcode, "%s", cases[i].name);
}

static void test_pairs(void) {
    for (int i = 0; i < 4; i++) {
        add((0x1234 << 8) | (0x01 | (i << 4)), "LXI %s, 0x1234", pairs[i]);
        add(0x03 | (i << 4), "INX %s", pairs[i]);
        add(0x0B | (i << 4), "DCX %s", pairs[i]);
        add(0x09 | (i << 4), "DAD %s", pairs[i]);
    }

    add(0x02, "STAX B");
    add(0x12, "STAX D");

    add(0x0A, "LDAX B");
    add(0x1A, "LDAX D");
}

static void test_registers(void) {
    for (int i = 0; i < 8; i++) {
        add(0x04 | (i << 3), "INR %s", regs[i]);
        add(0x05 | (i << 3), "DCR %s", regs[i]);
        add((0xA5 << 8) | (0x06 | (i << 3)) | 0x00, "MVI %s, 0xA5", regs[i]);

        add(0x80 | i, "ADD %s", regs[i]);
        add(0x88 | i, "ADC %s", regs[i]);
        add(0x90 | i, "SUB %s", regs[i]);
        add(0x98 | i, "SBB %s", regs[i]);
        add(0xA0 | i, "ANA %s", regs[i]);
        add(0xA8 | i, "XRA %s", regs[i]);
        add(0xB0 | i, "ORA %s", regs[i]);
        add(0xB8 | i, "CMP %s", regs[i]);
    }

    // MOV register-to-register.
    for (int dst = 0; dst < 8; dst++) {
        for (int src = 0; src < 8; src++) {
            if (dst == 6 && src == 6)
                continue;

            uint8_t opcode = 0x40 | (dst << 3) | src;

            add(opcode, "MOV %s, %s", regs[dst], regs[src]);
        }
    }
}

static void test_immediate(void) {
    struct {
        const char *name;
        uint8_t opcode;
    } cases[] = {
        {"ADI", 0xC6},
        {"ACI", 0xCE},
        {"SUI", 0xD6},
        {"SBI", 0xDE},
        {"ANI", 0xE6},
        {"XRI", 0xEE},
        {"ORI", 0xF6},
        {"CPI", 0xFE},
    };

    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        add(cases[i].opcode | 0xA500, "%s 0xA5", cases[i].name);
    }
}

static void test_address(void) {
    add(0x123422, "SHLD 0x1234");
    add(0x12342A, "LHLD 0x1234");
    add(0x123432, "STA 0x1234");
    add(0x12343A, "LDA 0x1234");

    add(0x1234C3, "JMP 0x1234");
    add(0x1234CD, "CALL 0x1234");
}

static void test_conditions(void) {
    for (int i = 0; i < 8; i++) {
        uint8_t jmp = 0xC2 | (i << 3);
        uint8_t call = 0xC4 | (i << 3);
        uint8_t ret = 0xC0 | (i << 3);

        add(0x123400 | jmp, "J%s 0x1234", conditions[i]);
        add(0x123400 | call, "C%s 0x1234", conditions[i]);
        add(ret, "R%s", conditions[i]);
    }
}

static void test_stack(void) {
    for (int i = 0; i < 4; i++) {
        add(0xC1 | (i << 4), "POP %s", stack_pairs[i]);
        add(0xC5 | (i << 4), "PUSH %s", stack_pairs[i]);
    }
}

static void test_io(void) {
    add(0xA5DB, "IN 0xA5");
    add(0xA5D3, "OUT 0xA5");
}

static void test_rst(void) {
    for (int i = 0; i < 8; i++) {
        add(0xC7 | (i << 3), "RST 0x%X", i);
    }
}

// MAIN

int main(void) {
    test_single();
    test_pairs();
    test_registers();
    test_immediate();
    test_address();
    test_conditions();
    test_stack();
    test_io();
    test_rst();

    run();

    return failed != 0;
}
