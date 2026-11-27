#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "i8080.h"
#include "disassembler.h"

struct DInstr disassembler_decode(const uint8_t instr[3]) {
  uint8_t opcode = instr[0];
  enum EncType enc_t = i8080_opcode_enc(opcode);

  struct DInstr d_instr = {0};
  d_instr.opcode = opcode;
  d_instr.reg_1 = REG_NONE;
  d_instr.reg_2 = REG_NONE;

  if (enc_t == ENC_NONE) {
    d_instr.type = ENC_NONE;
    d_instr.size = 1;
    return d_instr;
  }

  int reg_1, reg_2, num;
  reg_1 = reg_2 = num = -1;

  switch (enc_t) {
    case ENC_NONE:
    case ENC_FIXED:
      break;
    case ENC_REG_PAIR_SP_IMM16:
      reg_1 = (opcode >> 4) & 0x03;
      /* Falls through */
    case ENC_ADDR16:
      num = instr[2] << 8 | instr[1];
      break;
    case ENC_REG_REG:
      reg_1 = (opcode >> 3) & 0x07;
      reg_2 = opcode & 0x07;
      break;
    case ENC_REG_ID:
      reg_1 = (opcode >> 3) & 0x07;      
      break;
    case ENC_REG_ARITH:
      reg_1 = opcode & 0x07;      
      break;
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      reg_1 = (opcode >> 4) & 0x03;
      break;
    case ENC_REG_IMM8:
      reg_1 = (opcode >> 3) & 0x07;
      num = instr[1];
      break;
    case ENC_IMM8:
      num = instr[1];
      break;
    case ENC_RST:
      num = (opcode >> 3) & 0x07;
      break;
    default:
      break;
  }

  d_instr.type = enc_t;
  d_instr.reg_1 = (reg_1 >= 0) ? (int8_t)reg_1 : REG_NONE;
  d_instr.reg_2 = (reg_2 >= 0) ? (int8_t)reg_2 : REG_NONE;
  d_instr.num = (num >= 0) ? (uint16_t)num : 0;
  d_instr.size = i8080_instr_size(enc_t);

  return d_instr;
}

// Print bytes.
void disassembler_printb(struct DInstr d_instr) {
  if (d_instr.size == 1) {
    printf("%02X      ", d_instr.opcode);
  } else if (d_instr.size == 2) {
    printf("%02X %02X   ", d_instr.opcode, d_instr.num);
  } else if (d_instr.size == 3) {
    printf("%02X %02X %02X",
        d_instr.opcode,
        d_instr.num & 0xFF, // First byte in memory
        (d_instr.num >> 8) & 0xFF // Second byte in memory
    );
  }
  printf(" : ");
}

// Print formatted.
void disassembler_printf(struct DInstr d_instr) {
  if (d_instr.type == ENC_NONE) {
    printf("NONE");
    return;
  }

  const char *mnemonic = i8080_opcode_mnemonic(d_instr.opcode);
  if (!mnemonic) {
    printf("DB 0x%02X", d_instr.opcode);
    return;
  }

  const char *r1 = (d_instr.reg_1 >= 0) ? i8080_code_reg((uint8_t)d_instr.reg_1, d_instr.type) : NULL;
  const char *r2 = (d_instr.reg_2 >= 0) ? i8080_code_reg((uint8_t)d_instr.reg_2, d_instr.type) : NULL;

  printf("%s", mnemonic);

  switch (d_instr.type) {
    case ENC_NONE:
    case ENC_FIXED:
      break;

    case ENC_REG_REG:
      printf(" %s, %s", r1, r2);
      break;

    case ENC_REG_ID:
    case ENC_REG_ARITH:
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      printf(" %s", r1);
      break;

    case ENC_REG_PAIR_SP_IMM16:
      printf(" %s, 0x%04X", r1, d_instr.num);
      break;

    case ENC_REG_IMM8:
      printf(" %s, 0x%02X", r1, d_instr.num);
      break;

    case ENC_IMM8:
      printf(" 0x%02X", d_instr.num);
      break;

    case ENC_ADDR16:
      printf(" 0x%04X", d_instr.num);
      break;

    case ENC_RST:
      printf(" %u", d_instr.num);
      break;
  }
}
