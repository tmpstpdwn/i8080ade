#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#include "i8080/i8080.h"
#include "disassembler.h"

bool disassemble_file(const char *in) {
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
    struct I8080Instr d_instr = i8080_decode_instr(instr);
    printf("0x%04lX : ", pc);
    i8080_printb_instr(d_instr);
    i8080_printf_instr(d_instr);
    putchar('\n');
    pc += (d_instr.size == 0)? 1: d_instr.size;
  }

  free(rom_buff);
  fclose(fp);

  return true;
}
