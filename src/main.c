#include <stdio.h>
#include <string.h>

#include "assembler.h"
#include "disassembler.h"

static void print_usage(const char *program) {
  printf("Usage:\n");
  printf("  %s -a <input.asm> <output.bin>\n", program);
  printf("  %s -d <input.bin>\n", program);
  printf("  %s -h | --help\n", program);

  printf("\nOptions:\n");
  printf("  -a    Assemble a source file\n");
  printf("  -d    Disassemble a binary file\n");
  printf("  -h    Display this help message\n");
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 ||
        strcmp(argv[1], "--help") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "-a") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Error: Invalid arguments for -a\n");
            print_usage(argv[0]);
            return 1;
        }

        return assemble_rom(argv[2], argv[3]);
    }

    if (strcmp(argv[1], "-d") == 0) {
        if (argc != 3) {
            fprintf(stderr, "Error: Invalid arguments for -d\n");
            print_usage(argv[0]);
            return 1;
        }

        return disassemble_rom(argv[2]);
    }

    fprintf(stderr, "Error: Unknown option '%s'\n", argv[1]);
    print_usage(argv[0]);
    return 1;
}
