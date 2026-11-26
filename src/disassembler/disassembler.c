#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "i8080.h"
#include "disassembler.h"

struct DInstr disassembler_decode(uint8_t instr[3]) {
  uint8_t opcode = instr[0];
  enum EncType enc_t = i8080_opcode_enc(opcode);

  struct DInstr d_instr = {0};

  if (enc_t == ENC_NONE) return d_instr;

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
  d_instr.opcode_mnemonic = i8080_opcode_mnemonic(opcode);
  d_instr.reg_1 = (reg_1 >= 0)? i8080_code_reg(reg_1, enc_t): NULL;
  d_instr.reg_2 = (reg_2 >= 0)? i8080_code_reg(reg_2, enc_t): NULL;
  d_instr.num = (num >= 0)? num: 0;
  d_instr.size = i8080_instr_size(enc_t);

  return d_instr;
}

void disassembler_print(struct DInstr d_instr) {
  if (d_instr.type == ENC_NONE) {
    printf("NONE\n");
    return;
  }

  printf("%s", d_instr.opcode_mnemonic);

  switch (d_instr.type) {
    case ENC_NONE:
    case ENC_FIXED:
      break;

    case ENC_REG_REG:
      printf(" %s, %s", d_instr.reg_1, d_instr.reg_2);
      break;

    case ENC_REG_ID:
    case ENC_REG_ARITH:
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      printf(" %s", d_instr.reg_1);
      break;

    case ENC_REG_PAIR_SP_IMM16:
    case ENC_REG_IMM8:
      printf(" %s, 0x%04X", d_instr.reg_1, d_instr.num);
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

  printf("\n");
}
