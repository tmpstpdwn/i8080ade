#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

#include "i8080.h"

struct DInstr {
  enum EncType type;
  const char *opcode_mnemonic;
  const char *reg_1, *reg_2;
  uint16_t num;
  uint8_t size;
};

struct DInstr disassembler_decode(uint8_t instr[3]);
void disassembler_print(struct DInstr d_instr);

#endif
