#ifndef OPCODE_H
#define OPCODE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

enum EncType {
  ENC_NONE,
  ENC_FIXED,
  ENC_REG_REG,
  ENC_REG_ID, // INR, DCR
  ENC_REG_ARITH, // Arithmetic
  ENC_REG_PAIR_SP,
  ENC_REG_PAIR_BD,
  ENC_REG_PAIR_PSW,
  ENC_REG_PAIR_SP_IMM16, // LXI
  ENC_REG_IMM8, // MVI
  ENC_IMM8,
  ENC_ADDR16,
  ENC_RST
};

struct OpcodeRec {
  char *name;
  enum EncType enc_t;
  uint8_t opcode;
};

bool opcode_rec_get(const char *mnemonic, struct OpcodeRec *oprec);
int opcode_reg_code(const char *reg, enum EncType enc_t);
uint8_t opcode_instr_size(enum EncType enc_t);

#endif
