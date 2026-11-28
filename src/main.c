#include <stdio.h>
#include <string.h>

#include "assembler/assembler.h"
#include "disassembler/disassembler.h"

static void print_usage(FILE *out, const char *prog) {
  fprintf(out,
          "Usage:\n"
          "  %s -a <input.asm> <output.bin>   Assemble a source file\n"
          "  %s -d <input.bin>                Disassemble a binary file\n"
          "  %s -h | --help                   Show this help\n",
          prog, prog, prog
        );
}

int main(int argc, char **argv) {
  const char *opt = argc > 1 ? argv[1] : "";

  if (!strcmp(opt, "-h") || !strcmp(opt, "--help")) {
    print_usage(stdout, argv[0]);
    return 0;
  }

  if (!strcmp(opt, "-a") && argc == 4) return assemble_file(argv[2], argv[3]) ? 0 : 1;
  if (!strcmp(opt, "-d") && argc == 3) return disassemble_file(argv[2]) ? 0 : 1;

  print_usage(stderr, argv[0]);
  return 1;
}
