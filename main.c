#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "assembler.h"
#include "disassembler.h"

static bool assemble_rom(FILE *fp) {
  char line[256];

  while (fgets(line, sizeof(line), fp) != NULL) {
    assemble(line);
  }

  return true;
}

static bool disassemble_rom(FILE *fp) {
  fseek(fp, 0, SEEK_END);
  int fsize = ftell(fp);
  rewind(fp);

  if (fsize <= 0) {
    printf("File size determination failed.\n");
    fclose(fp);
    return false;
  }

  uint8_t *rom_buff = NULL;
  size_t pc = 0;

  rom_buff = malloc(fsize);

  if (rom_buff == NULL || fread(rom_buff, 1, fsize, fp) != fsize) {
    printf("Loading rom to memory failed.\n");
    free(rom_buff);
    return false;
  }

  // Disassembly of the entire rome.
  while (pc < fsize) {
    int size = disassemble(rom_buff, pc);
    pc += size;
  }

  free(rom_buff);

  return true;
}

int main(int argc, char** argv) {
  if (argc == 1) {
    printf("Args needed!\n");
    return 1;
  }

  // File buffering.
  FILE *fp = fopen(argv[1], "rb");

  if (!fp) {
    printf("File opening failed.\n");
    return 1;
  }

  // Do stuff.

  fclose(fp);

  return 0;
}
