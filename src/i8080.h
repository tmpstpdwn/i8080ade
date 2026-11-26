#ifndef I8080_H
#define I8080_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "util/slice.h"

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

uint8_t i8080_instr_size(enum EncType enc_t);
enum EncType i8080_opcode_enc(uint8_t opcode);
bool i8080_opcode_rec(struct Slice mnemonic, struct OpcodeRec *oprec);
int i8080_reg_code(struct Slice reg, enum EncType enc_t);
const char *i8080_code_reg(uint8_t n, enum EncType type);
const char *i8080_opcode_mnemonic(uint8_t opcode);

#endif
