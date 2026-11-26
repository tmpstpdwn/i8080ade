#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "util/vector.h"
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

static char *read_file(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;

  char *text = NULL;
  long size = -1;
  if (fseek(fp, 0, SEEK_END) == 0) size = ftell(fp);

  if (size >= 0 && fseek(fp, 0, SEEK_SET) == 0 &&
      (text = malloc((size_t)size + 1))) {
    size_t n = fread(text, 1, (size_t)size, fp);
    text[n] = '\0';
  }

  fclose(fp);
  return text;
}

static int write_file(const char *path, const void *data, size_t len) {
  FILE *fp = fopen(path, "wb");
  if (!fp) return 0;
  int ok = fwrite(data, 1, len, fp) == len;
  return fclose(fp) == 0 && ok;
}

static bool assemble_file(const char *in, const char *out) {
  char *text = read_file(in);
  if (!text) {
    fprintf(stderr, "Error: cannot read '%s'\n", in);
    return false;
  }

  struct Vector *buf = vector_new(sizeof(uint8_t));
  if (!buf) {
    fprintf(stderr, "Error: out of memory\n");
    free(text);
    return false;
  }

  if (assemble_source(text, in, buf)) {
    if (write_file(out, vector_get_data(buf), vector_len(buf))) {
      printf("Written to %s\n", out);
    } else {
      fprintf(stderr, "Error: cannot write '%s'\n", out);
    }
  }

  free(text);
  vector_free(buf);
  return true;
}

static bool disassemble_file(const char *in) {
  FILE *fp = fopen(in, "rb");

  if (!fp) {
    printf("Opening file \'%s\' failed.\n", in);
    return false;
  }

  fseek(fp, 0, SEEK_END);
  int sz = ftell(fp);
  rewind(fp);

  if (sz <= 0) {
    printf("File size determination failed.\n");
    fclose(fp);
    return false;
  }

  uint64_t fsize = (uint64_t)sz;

  uint8_t *rom_buff = NULL;
  size_t pc = 0;

  rom_buff = malloc(sz);

  if (rom_buff == NULL || fread(rom_buff, 1, fsize, fp) != fsize) {
    printf("Loading rom into to memory failed.\n");
    free(rom_buff);
    fclose(fp);
    return false;
  }

  // Disassembly of the entire rome.
  while (pc < fsize) {
    uint8_t instr[3] = {0};
    instr[0] = rom_buff[pc];
    if (pc + 1 < fsize) instr[1] = rom_buff[pc + 1];
    if (pc + 2 < fsize) instr[2] = rom_buff[pc + 2];
    struct DInstr d_instr = disassembler_decode(instr);
    disassembler_print(d_instr);
    pc += (d_instr.size == 0)? 1: d_instr.size;
  }

  free(rom_buff);
  fclose(fp);

  return true;
}

int main(int argc, char **argv) {
  const char *opt = argc > 1 ? argv[1] : "";

  if (!strcmp(opt, "-h") || !strcmp(opt, "--help")) {
    print_usage(stdout, argv[0]);
    return 0;
  }

  if (!strcmp(opt, "-a") && argc == 4) return assemble_file(argv[2], argv[3]);
  if (!strcmp(opt, "-d") && argc == 3) return disassemble_file(argv[2]);

  print_usage(stderr, argv[0]);
  return 1;
}
