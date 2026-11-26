#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "util/slice.h"
#include "i8080.h"

static const char *r[] = {"B", "C", "D", "E", "H", "L", "M", "A"};
static const char *rp_sp[] = {"B", "D", "H", "SP"};
static const char *rp_psw[] = {"B", "D", "H", "PSW"};
static const char *rp_bd[] = {"B", "D"};

static const char *arith[] = {
  "ADD", "ADC", "SUB", "SBB",
  "ANA", "XRA", "ORA", "CMP"
};

static const char *imm_arith[] = {
  "ADI", "ACI", "SUI", "SBI",
  "ANI", "XRI", "ORI", "CPI"
};

static const char *ret_cc[] = {
    "RNZ", "RZ", "RNC", "RC",
    "RPO", "RPE", "RP", "RM"
};

static const char *jmp_cc[] = {
    "JNZ", "JZ", "JNC", "JC",
    "JPO", "JPE", "JP", "JM"
};

static const char *call_cc[] = {
    "CNZ", "CZ", "CNC", "CC",
    "CPO", "CPE", "CP", "CM"
};

static const struct OpcodeRec opcode_rec[] = {
  {"NOP",       ENC_FIXED,                  0x00},
  {"LXI",       ENC_REG_PAIR_SP_IMM16,      0x01},
  {"STAX",      ENC_REG_PAIR_BD,            0x02},
  {"INX",       ENC_REG_PAIR_SP,            0x03},
  {"INR",       ENC_REG_ID,                 0x04},
  {"DCR",       ENC_REG_ID,                 0x05},
  {"MVI",       ENC_REG_IMM8,               0x06},
  {"RLC",       ENC_FIXED,                  0x07},
  {"DAD",       ENC_REG_PAIR_SP,            0x09},
  {"LDAX",      ENC_REG_PAIR_BD,            0x0A},
  {"DCX",       ENC_REG_PAIR_SP,            0x0B},
  {"RRC",       ENC_FIXED,                  0x0F},
  {"RAL",       ENC_FIXED,                  0x17},
  {"RAR",       ENC_FIXED,                  0x1F},
  {"SHLD",      ENC_ADDR16,                 0x22},
  {"DAA",       ENC_FIXED,                  0x27},
  {"LHLD",      ENC_ADDR16,                 0x2A},
  {"CMA",       ENC_FIXED,                  0x2F},
  {"STA",       ENC_ADDR16,                 0x32},
  {"STC",       ENC_FIXED,                  0x37},
  {"LDA",       ENC_ADDR16,                 0x3a},
  {"CMC",       ENC_FIXED,                  0x3F},
  {"MOV",       ENC_REG_REG,                0x40},
  {"HLT",       ENC_FIXED,                  0x76},
  {"ADD",       ENC_REG_ARITH,              0x80},
  {"ADC",       ENC_REG_ARITH,              0x88},
  {"SUB",       ENC_REG_ARITH,              0x90},
  {"SBB",       ENC_REG_ARITH,              0x98},
  {"ANA",       ENC_REG_ARITH,              0xA0},
  {"XRA",       ENC_REG_ARITH,              0xA8},
  {"ORA",       ENC_REG_ARITH,              0xB0},
  {"CMP",       ENC_REG_ARITH,              0xB8},
  {"RNZ",       ENC_FIXED,                  0xC0},
  {"POP",       ENC_REG_PAIR_PSW,           0xC1},
  {"JNZ",       ENC_ADDR16,                 0xC2},
  {"JMP",       ENC_ADDR16,                 0xC3},
  {"CNZ",       ENC_ADDR16,                 0xC4},
  {"PUSH",      ENC_REG_PAIR_PSW,           0xc5},
  {"ADI",       ENC_IMM8,                   0xC6},
  {"RST",       ENC_RST,                    0xC7},
  {"RZ",        ENC_FIXED,                  0xC8},
  {"RET",       ENC_FIXED,                  0xC9},
  {"JZ",        ENC_ADDR16,                 0xCA},
  {"CZ",        ENC_ADDR16,                 0xCC},
  {"CALL",      ENC_ADDR16,                 0xCD},
  {"ACI",       ENC_IMM8,                   0xCE},
  {"RNC",       ENC_FIXED,                  0xD0},
  {"JNC",       ENC_ADDR16,                 0xD2},
  {"OUT",       ENC_IMM8,                   0xD3},
  {"CNC",       ENC_ADDR16,                 0xD4},
  {"SUI",       ENC_IMM8,                   0xD6},
  {"RC",        ENC_FIXED,                  0xD8},
  {"JC",        ENC_ADDR16,                 0xDA},
  {"IN",        ENC_IMM8,                   0xDB},
  {"CC",        ENC_ADDR16,                 0xDC},
  {"SBI",       ENC_IMM8,                   0xDE},
  {"RPO",       ENC_FIXED,                  0xE0},
  {"JPO",       ENC_ADDR16,                 0xE2},
  {"XTHL",      ENC_FIXED,                  0xE3},
  {"CPO",       ENC_ADDR16,                 0xE4},
  {"ANI",       ENC_IMM8,                   0xE6},
  {"RPE",       ENC_FIXED,                  0xE8},
  {"PCHL",      ENC_FIXED,                  0xE9},
  {"JPE",       ENC_ADDR16,                 0xEA},
  {"XCHG",      ENC_FIXED,                  0xEB},
  {"CPE",       ENC_ADDR16,                 0xEC},
  {"XRI",       ENC_IMM8,                   0xEE},
  {"RP",        ENC_FIXED,                  0xF0},
  {"JP",        ENC_ADDR16,                 0xF2},
  {"DI",        ENC_FIXED,                  0xF3},
  {"CP",        ENC_ADDR16,                 0xF4},
  {"ORI",       ENC_IMM8,                   0xF6},
  {"RM",        ENC_FIXED,                  0xF8},
  {"SPHL",      ENC_FIXED,                  0xF9},
  {"JM",        ENC_ADDR16,                 0xFA},
  {"EI",        ENC_FIXED,                  0xFB},
  {"CM",        ENC_ADDR16,                 0xFC},
  {"CPI",       ENC_IMM8,                   0xFE},
  {NULL,        ENC_NONE,                   0x00}
};

uint8_t i8080_instr_size(enum EncType enc_t) {
  uint8_t size = 0;
  switch (enc_t) {
    case ENC_NONE:
      break;

    case ENC_RST:
    case ENC_FIXED:
    case ENC_REG_ID:
    case ENC_REG_REG:
    case ENC_REG_ARITH:
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      size = 1;
      break;

    case ENC_IMM8:
    case ENC_REG_IMM8:
      size = 2;
      break;

    case ENC_ADDR16:
    case ENC_REG_PAIR_SP_IMM16:
      size = 3;
      break;
  }

  return size;
}

enum EncType i8080_opcode_enc(uint8_t opcode) {
  const char *mnemonic = i8080_opcode_mnemonic(opcode);
  if (!mnemonic) return ENC_NONE;

  struct Slice slice = slice_from(mnemonic, 0, strlen(mnemonic));

  struct OpcodeRec oprec;
  if (i8080_opcode_rec(slice, &oprec))
    return oprec.enc_t;
 return ENC_NONE;
}

bool i8080_opcode_rec(struct Slice mnemonic, struct OpcodeRec *oprec) {
  if (!oprec || !mnemonic.text || mnemonic.len == 0 || mnemonic.len > 4)
    return false;

  for (int i = 0; opcode_rec[i].name != NULL; i++) {
    struct Slice slice = slice_from(opcode_rec[i].name, 0, strlen(opcode_rec[i].name));
    if (slice_eq(mnemonic, slice, true)) {
      *oprec = opcode_rec[i];
      return true;
    }
  }

  return false;
}

int i8080_reg_code(struct Slice reg, enum EncType enc_t) {
  if (!reg.text || reg.len == 0 || reg.len > 3)
    return -1;

  const char **table = NULL;
  int count = 0;

  switch (enc_t) {
    case ENC_REG_REG:
    case ENC_REG_ID:
    case ENC_REG_ARITH:
    case ENC_REG_IMM8:
      table = r;
      count = 8;
      break;

    case ENC_REG_PAIR_BD:
      table = rp_bd;
      count = 2;
      break;

    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_SP_IMM16:
      table = rp_sp;
      count = 4;
      break;

    case ENC_REG_PAIR_PSW:
      table = rp_psw;
      count = 4;
      break;

    default:
      return -1;
  }

  for (int i = 0; i < count; i++) {
    struct Slice slice = slice_from(table[i], 0, strlen(table[i]));
    if (slice_eq(reg, slice, true))
      return i;
  }

  return -1;
}

const char *i8080_code_reg(uint8_t n, enum EncType type) {
  switch (type) {
    case ENC_REG_REG:
    case ENC_REG_ID:
    case ENC_REG_IMM8:
    case ENC_REG_ARITH:
      return n < 8 ? r[n] : NULL;
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_SP_IMM16:
      return n < 4 ? rp_sp[n] : NULL;
    case ENC_REG_PAIR_PSW:
      return n < 4 ? rp_psw[n] : NULL;
    case ENC_REG_PAIR_BD:
      return n < 2 ? rp_bd[n] : NULL;
    default:
      return NULL;
  }
}

const char *i8080_opcode_mnemonic(uint8_t opcode) {
  if ((opcode & 0xC7) == 0x04) return "INR";
  if ((opcode & 0xC7) == 0x05) return "DCR";
  if ((opcode & 0xC7) == 0x06) return "MVI";
  if ((opcode & 0xC0) == 0x40 && opcode != 0x76) return "MOV";
  if (opcode == 0x76) return "HLT";
  if ((opcode & 0xC0) == 0x80) return arith[(opcode >> 3) & 7];
  if ((opcode & 0xCF) == 0x01) return "LXI";
  if ((opcode & 0xCF) == 0x03) return "INX";
  if ((opcode & 0xCF) == 0x0B) return "DCX";
  if ((opcode & 0xCF) == 0x09) return "DAD";
  if ((opcode & 0xEF) == 0x02) return "STAX";
  if ((opcode & 0xEF) == 0x0A) return "LDAX";
  if ((opcode & 0xCF) == 0xC5) return "PUSH";
  if ((opcode & 0xCF) == 0xC1) return "POP";
  if ((opcode & 0xC7) == 0xC7) return "RST";
  if ((opcode & 0xC7) == 0xC6) return imm_arith[(opcode >> 3) & 7];
  if ((opcode & 0xC7) == 0xC0) return ret_cc[(opcode >> 3) & 7];
  if ((opcode & 0xC7) == 0xC2 || opcode == 0xC3)
    return opcode == 0xC3 ? "JMP" : jmp_cc[(opcode >> 3) & 7];
  if ((opcode & 0xC7) == 0xC4 || opcode == 0xCD)
    return opcode == 0xCD ? "CALL" : call_cc[(opcode >> 3) & 7];

  switch (opcode) {
    case 0x22: return "SHLD";
    case 0x2A: return "LHLD";
    case 0x32: return "STA";
    case 0x3A: return "LDA";
  }

  switch (opcode) {
    case 0x00: return "NOP";
    case 0x07: return "RLC";
    case 0x0F: return "RRC";
    case 0x17: return "RAL";
    case 0x1F: return "RAR";
    case 0x27: return "DAA";
    case 0x2F: return "CMA";
    case 0x37: return "STC";
    case 0x3F: return "CMC";
    case 0xC9: return "RET";
    case 0xD3: return "OUT";
    case 0xDB: return "IN";
    case 0xE3: return "XTHL";
    case 0xE9: return "PCHL";
    case 0xEB: return "XCHG";
    case 0xF3: return "DI";
    case 0xF9: return "SPHL";
    case 0xFB: return "EI";
  }

  return NULL;
}
