#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "assembler.h"
#include "disassembler.h"

static bool assemble_rom(FILE *fpr, FILE *fpw) {
  char line[256];

  uint8_t *buffer = NULL;
  size_t buffer_i = 0;
  size_t buffer_len = 0;

  int i = 1;

  while (fgets(line, sizeof(line), fpr) != NULL) {
    uint32_t assembled_instr = assemble(line, i++);
    uint8_t size = (assembled_instr & 0xFF000000) >> 24;

    while (size) {
      if (buffer_i >= buffer_len) {
        uint8_t *tmp = realloc(buffer, sizeof(uint8_t) * (buffer_len + 2048));
        if (!tmp) {
          free(buffer);
          return false;
        }
        buffer = tmp;
        buffer_len += 2048;
      }
      buffer[buffer_i++] = assembled_instr & 0xFF;
      assembled_instr = assembled_instr >> 8;
      size--;
    }

  }

  fwrite(buffer, sizeof(uint8_t), buffer_i, fpw);
  free(buffer);
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

  if (strcmp(argv[1], "-a") == 0) {
    if (argc != 4) {
      printf("Invalid arguments for -a\n");
      return 1;
    }

    FILE *fpr = fopen(argv[2], "rb");
    FILE *fpw = fopen(argv[3], "wb");

    if (!fpr || !fpw) {
      printf("File opening failed.\n");
      return 1;
    }

    assemble_rom(fpr, fpw);

    fclose(fpr);
    fclose(fpw);
  }

  else if (strcmp(argv[1], "-d") == 0) {
    if (argc != 3) {
      printf("%d\n", argc);
      printf("Invalid arguments for -d\n");
      return 1;
    }

    FILE *fp = fopen(argv[2], "rb");

    if (!fp) {
      printf("File opening failed.\n");
      return 1;
    }

    disassemble_rom(fp);

    fclose(fp);
  }

  return 0;
}
