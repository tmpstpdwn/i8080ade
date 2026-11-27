#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

#include "i8080.h"

#define REG_NONE (-1)

struct DInstr {
  uint8_t opcode;
  enum EncType type;
  int8_t reg_1;
  int8_t reg_2;
  uint16_t num;
  uint8_t size;
};

struct DInstr disassembler_decode(const uint8_t instr[3]);
void disassembler_printb(struct DInstr d_instr);
void disassembler_printf(struct DInstr d_instr);

#endif
