#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

enum OpcodeType {
  OP_FIXED,
  OP_REG_REG,
  OP_REG_ID,
  OP_REG,
  OP_REG_PAIR_SP,
  OP_REG_PAIR_BD,
  OP_REG_PAIR_PSW,
  OP_REG_IMM16,
  OP_REG_IMM8,
  OP_IMM8,
  OP_ADDR16
};

struct OpRec {
  char *name;
  enum OpcodeType type;
  uint8_t opcode;
};

static struct OpRec opcode_table[] = {
  {"NOP",      OP_FIXED,          0x00},
  {"LXI",      OP_REG_IMM16,       0x01},
  {"STAX",     OP_REG_PAIR_BD,    0x02},
  {"INX",      OP_REG_PAIR_SP,       0x03},
  {"INR",      OP_REG_ID,         0x04},
  {"DCR",      OP_REG_ID,         0x05},
  {"MVI",      OP_REG_IMM8,       0x06},
  {"RLC",      OP_FIXED,          0x07},
  {"DAD",      OP_REG_PAIR_SP,       0x09},
  {"LDAX",     OP_REG_PAIR_BD,    0x0A},
  {"DCX",      OP_REG_PAIR_SP,       0x0B},
  {"RRC",      OP_FIXED,          0x0F},
  {"RAL",      OP_FIXED,          0x17},
  {"RAR",      OP_FIXED,          0x1F},
  {"SHLD",     OP_ADDR16,         0x22},
  {"DAA",      OP_FIXED,          0x27},
  {"LHLD",     OP_ADDR16,         0x2A},
  {"CMA",      OP_FIXED,          0x2F},
  {"STA",      OP_ADDR16,         0x32},
  {"STC",      OP_FIXED,          0x37},
  {"LDA",      OP_ADDR16,         0x3a},
  {"CMC",      OP_FIXED,          0x3F},
  {"MOV",      OP_REG_REG,        0x40},
  {"HLT",      OP_FIXED,          0x76},
  {"ADD",      OP_REG,            0x80},
  {"ADC",      OP_REG,            0x88},
  {"SUB",      OP_REG,            0x90},
  {"SBB",      OP_REG,            0x98},
  {"ANA",      OP_REG,            0xA0},
  {"XRA",      OP_REG,            0xA8},
  {"ORA",      OP_REG,            0xB0},
  {"CMP",      OP_REG,            0xB8},
  {"RNZ",      OP_FIXED,          0xC0},
  {"POP",      OP_REG_PAIR_PSW,   0xC1},
  {"JNZ",      OP_ADDR16,         0xC2},
  {"JMP",      OP_ADDR16,         0xC3},
  {"CNZ",      OP_ADDR16,         0xC4},
  {"PUSH",     OP_REG_PAIR_PSW,   0xc5},
  {"ADI",      OP_IMM8,           0xC6},
  {"RST",      OP_IMM8,           0xC7},
  {"RZ",       OP_FIXED,          0xC8},
  {"RET",      OP_FIXED,          0xC9},
  {"JZ",       OP_ADDR16,         0xCA},
  {"CZ",       OP_ADDR16,         0xCC},
  {"CALL",     OP_ADDR16,         0xCD},
  {"ACI",      OP_IMM8,           0xCE},
  {"RNC",      OP_FIXED,          0xD0},
  {"JNC",      OP_ADDR16,         0xD2},
  {"OUT",      OP_IMM8,           0xD3},
  {"CNC",      OP_ADDR16,         0xD4},
  {"SUI",      OP_IMM8,           0xD6},
  {"RC",       OP_FIXED,          0xD8},
  {"JC",       OP_ADDR16,         0xDA},
  {"IN",       OP_IMM8,           0xDB},
  {"CC",       OP_ADDR16,         0xDC},
  {"SBI",      OP_IMM8,           0xDE},
  {"RPO",      OP_FIXED,          0xE0},
  {"JPO",      OP_ADDR16,         0xE2},
  {"XTHL",     OP_FIXED,          0xE3},
  {"CPO",      OP_ADDR16,         0xE4},
  {"ANI",      OP_IMM8,           0xE6},
  {"RPE",      OP_FIXED,          0xE8},
  {"PCHL",     OP_FIXED,          0xE9},
  {"JPE",      OP_ADDR16,         0xEA},
  {"XCHG",     OP_FIXED,          0xEB},
  {"CPE",      OP_ADDR16,         0xEC},
  {"XRI",      OP_IMM8,           0xEE},
  {"RP",       OP_FIXED,          0xF0},
  {"JP",       OP_ADDR16,         0xF2},
  {"DI",       OP_FIXED,          0xF3},
  {"CP",       OP_ADDR16,         0xF4},
  {"ORI",      OP_IMM8,           0xF6},
  {"RM",       OP_FIXED,          0xF8},
  {"SPHL",     OP_FIXED,          0xF9},
  {"JM",       OP_ADDR16,         0xFA},
  {"EI",       OP_FIXED,          0xFB},
  {"CM",       OP_ADDR16,         0xFC},
  {"CPI",      OP_IMM8,           0xFE},
  {NULL}
};

static int register_code(char r) {
  switch (r) {
    case 'B':
      return 0;
    case 'C':
      return 1;
    case 'D':
      return 2;
    case 'E':
      return 3;
    case 'H':
      return 4;
    case 'L':
      return 5;
    case 'M':
      return 6;
    case 'A':
      return 7;
    default:
      return -1;
  }
}

static int operand_code(char *r, enum OpcodeType op_t) {
  if (!r) return -1;

  int len = strlen(r);
  if (len == 0) return -1;

  int n = register_code(*r);

  switch (op_t) {
    case OP_REG_REG:
    case OP_REG_ID:
    case OP_REG:
    case OP_REG_IMM8: {
        if (len != 1) return -1;
        return n;
    }

    case OP_REG_PAIR_BD: {
        if (len == 1 && n % 2 == 0 && n < 4)
          return n / 2;
        return -1;
    }

    case OP_REG_PAIR_SP:
    case OP_REG_IMM16:
    case OP_REG_PAIR_PSW: {
        if (len == 1 && n % 2 == 0 && n < 6) {
          return n / 2;
        } else {
          char *s = (op_t == OP_REG_PAIR_PSW)? "PSW": "SP";
          if (strcmp(r, s) == 0) return 3;
        }
        return -1;
    }

    default: return -1;
  }
}

static void to_uppercase(char *s) {
  if (!s) return;

  while (*s) {
    if (isalpha(*s)) {
      *s = toupper(*s);
    }
    s++;
  }
}

static bool str_to_hex(const char *str, uint64_t *num) {
    char *endptr;
    uint64_t val;

    errno = 0;

    val = strtoull(str, &endptr, 16);

    if (errno == ERANGE) return false;

    if (endptr == str) return false;
    while (isspace((unsigned char)*endptr)) {
        endptr++;
    }
    if (*endptr != '\0') return false;

    *num = val;

    return true;
} 

static uint32_t assemble(char *instr) {
  uint32_t asmbld_instr = 0;

  char *token = strtok(instr, " ,");

  for (int i = 0; opcode_table[i].name != NULL; i++) {
    if (strcmp(token, opcode_table[i].name) == 0) {
      enum OpcodeType op_t = opcode_table[i].type;
      uint8_t opcode = opcode_table[i].opcode;

      char *operand1 = strtok(NULL, " ,");
      char *operand2 = strtok(NULL, " ,");

      int operand1_c = operand_code(operand1, op_t);
      int operand2_c = operand_code(operand2, op_t);

      switch (op_t) {

        case OP_FIXED:
          asmbld_instr = opcode;
          break;

        case OP_REG_REG:
          if (operand1_c == -1 || operand2_c == -1) break;
          asmbld_instr = opcode + (operand1_c << 3) + operand2_c;
          break;

        case OP_REG_ID:
          if (operand1_c == -1) break;
          asmbld_instr = opcode + (operand1_c << 3);
          break;

        case OP_REG:
          if (operand1_c == -1) break;
          asmbld_instr = opcode + operand1_c;
          break;

        case OP_REG_PAIR_SP:
        case OP_REG_PAIR_BD:
        case OP_REG_PAIR_PSW:
          if (operand1_c == -1) break;
          asmbld_instr = opcode + (operand1_c << 4);
          break;

        case OP_REG_IMM16: {
          if (operand1_c == -1) break;

          uint64_t val;
          if (str_to_hex(operand2, &val) == false) break;
          uint16_t val_16 = val & 0xFFFF;

          uint8_t final_opcode = opcode + (operand1_c << 4);
          uint16_t d16_little_endian = (val_16 & 0x00FF) << 8 | val_16 >> 8;

          asmbld_instr = ((uint32_t)final_opcode << 16) | d16_little_endian;
          break;
        }

        case OP_REG_IMM8: {
          if (operand1_c == -1) break;

          uint64_t val;
          if (str_to_hex(operand2, &val) == false) break;
          uint8_t d8 = val & 0xFF;

          uint8_t final_opcode = opcode + (operand1_c << 4);

          asmbld_instr = ((uint32_t)final_opcode << 8) | d8;
          break;
        }

        case OP_IMM8: {
          uint64_t val;

          if (str_to_hex(operand2, &val) == false) break;
          uint8_t d8 = val & 0xFF;

          asmbld_instr = ((uint32_t)opcode << 8) | d8;
          break;
        }

        case OP_ADDR16: {
          uint64_t val;

          if (str_to_hex(operand2, &val) == false) break;

          uint16_t val_16 = val & 0xFFFF;
          uint16_t d16_little_endian = (val_16 & 0x00FF) << 8 | val_16 >> 8;

          asmbld_instr = ((uint32_t)opcode << 16) | d16_little_endian;
          break;
        }
         
        default: break;
      }

    }
  }

  return asmbld_instr;
}  

static bool assemble_rom(FILE *fp) {
  char line[256];

  while (fgets(line, sizeof(line), fp) != NULL) {
    assemble(line);
  }

  return true;
}

static int dissassemble_instr(uint8_t *buff, size_t pc) {
  int size = 1;
  uint8_t instr = buff[pc];

  switch (instr) {
    case 0x00: printf("NOP\n"); break;
    case 0x01: printf("LXI B, 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x02: printf("STAX B\n"); break;
    case 0x03: printf("INX B\n"); break;
    case 0x04: printf("INR B\n"); break;
    case 0x05: printf("DCR B\n"); break;
    case 0x06: printf("MVI B, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x07: printf("RLC\n"); break;
    // 0x08 : invalid.
    case 0x09: printf("DAD B\n"); break;
    case 0x0A: printf("LDAX B\n"); break;
    case 0x0B: printf("DCX B\n"); break;
    case 0x0C: printf("INR C\n"); break;
    case 0x0D: printf("DCR C\n"); break;
    case 0x0E: printf("MVI C, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x0F: printf("RRC\n"); break;

    // 0x10 : invalid.
    case 0x11: printf("LXI D, 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x12: printf("STAX D\n"); break;
    case 0x13: printf("INX D\n"); break;
    case 0x14: printf("INR D\n"); break;
    case 0x15: printf("DCR D\n"); break;
    case 0x16: printf("MVI D, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x17: printf("RAL\n"); break;
    // 0x18 : invalid.
    case 0x19: printf("DAD D\n"); break;
    case 0x1A: printf("LDAX D\n"); break;
    case 0x1B: printf("DCX D\n"); break;
    case 0x1C: printf("INR E\n"); break;
    case 0x1D: printf("DCR E\n"); break;
    case 0x1E: printf("MVI E, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x1F: printf("RAR\n"); break;

    // 0x20 : invalid.
    case 0x21: printf("LXI H, 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x22: printf("SHLD 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x23: printf("INX H\n"); break;
    case 0x24: printf("INR H\n"); break;
    case 0x25: printf("DCR H\n"); break;
    case 0x26: printf("MVI H, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x27: printf("DAA\n"); break;
    // 0x28 : invalid.
    case 0x29: printf("DAD H\n"); break;
    case 0x2A: printf("LHLD 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x2B: printf("DCX H\n"); break;
    case 0x2C: printf("INR L\n"); break;
    case 0x2D: printf("DCR L\n"); break;
    case 0x2E: printf("MVI L, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x2F: printf("CMA\n"); break;

    // 0x30 : invalid.
    case 0x31: printf("LXI SP, 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x32: printf("STA 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x33: printf("INX SP\n"); break;
    case 0x34: printf("INR M\n"); break;
    case 0x35: printf("DCR M\n"); break;
    case 0x36: printf("MVI M, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x37: printf("STC\n"); break;
    // 0x38 : invalid.
    case 0x39: printf("DAD SP\n"); break;
    case 0x3A: printf("LDA 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0x3B: printf("DCX SP\n"); break;
    case 0x3C: printf("INR A\n"); break;
    case 0x3D: printf("DCR A\n"); break;
    case 0x3E: printf("MVI A, 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0x3F: printf("CMC\n"); break;

    case 0x40: printf("MOV B, B\n"); break;
    case 0x41: printf("MOV B, C\n"); break;
    case 0x42: printf("MOV B, D\n"); break;
    case 0x43: printf("MOV B, E\n"); break;
    case 0x44: printf("MOV B, H\n"); break;
    case 0x45: printf("MOV B, L\n"); break;
    case 0x46: printf("MOV B, M\n"); break;
    case 0x47: printf("MOV B, A\n"); break;

    case 0x48: printf("MOV C, B\n"); break;
    case 0x49: printf("MOV C, C\n"); break;
    case 0x4A: printf("MOV C, D\n"); break;
    case 0x4B: printf("MOV C, E\n"); break;
    case 0x4C: printf("MOV C, H\n"); break;
    case 0x4D: printf("MOV C, L\n"); break;
    case 0x4E: printf("MOV C, M\n"); break;
    case 0x4F: printf("MOV C, A\n"); break;

    case 0x50: printf("MOV D, B\n"); break;
    case 0x51: printf("MOV D, C\n"); break;
    case 0x52: printf("MOV D, D\n"); break;
    case 0x53: printf("MOV D, E\n"); break;
    case 0x54: printf("MOV D, L\n"); break;
    case 0x55: printf("MOV D, L\n"); break;
    case 0x56: printf("MOV D, M\n"); break;
    case 0x57: printf("MOV D, A\n"); break;

    case 0x58: printf("MOV E, B\n"); break;
    case 0x59: printf("MOV E, C\n"); break;
    case 0x5A: printf("MOV E, D\n"); break;
    case 0x5B: printf("MOV E, E\n"); break;
    case 0x5C: printf("MOV E, H\n"); break;
    case 0x5D: printf("MOV E, L\n"); break;
    case 0x5E: printf("MOV E, M\n"); break;
    case 0x5F: printf("MOV E, A\n"); break;

    case 0x60: printf("MOV H, B\n"); break;
    case 0x61: printf("MOV H, C\n"); break;
    case 0x62: printf("MOV H, D\n"); break;
    case 0x63: printf("MOV H, E\n"); break;
    case 0x64: printf("MOV H, H\n"); break;
    case 0x65: printf("MOV H, L\n"); break;
    case 0x66: printf("MOV H, M\n"); break;
    case 0x67: printf("MOV H, A\n"); break;

    case 0x68: printf("MOV L, B\n"); break;
    case 0x69: printf("MOV L, C\n"); break;
    case 0x6A: printf("MOV L, D\n"); break;
    case 0x6B: printf("MOV L, E\n"); break;
    case 0x6C: printf("MOV L, H\n"); break;
    case 0x6D: printf("MOV L, L\n"); break;
    case 0x6E: printf("MOV L, M\n"); break;
    case 0x6F: printf("MOV L, A\n"); break;

    case 0x70: printf("MOV M, B\n"); break;
    case 0x71: printf("MOV M, C\n"); break;
    case 0x72: printf("MOV M, D\n"); break;
    case 0x73: printf("MOV M, E\n"); break;
    case 0x74: printf("MOV M, H\n"); break;
    case 0x75: printf("MOV M, L\n"); break;

    case 0x76: printf("HLT\n"); break;

    case 0x77: printf("MOV M, A\n"); break;

    case 0x78: printf("MOV A, B\n"); break;
    case 0x79: printf("MOV A, C\n"); break;
    case 0x7A: printf("MOV A, D\n"); break;
    case 0x7B: printf("MOV A, E\n"); break;
    case 0x7C: printf("MOV A, H\n"); break;
    case 0x7D: printf("MOV A, L\n"); break;
    case 0x7E: printf("MOV A, M\n"); break;
    case 0x7F: printf("MOV A, A\n"); break;

    case 0x80: printf("ADD B\n"); break;
    case 0x81: printf("ADD C\n"); break;
    case 0x82: printf("ADD D\n"); break;
    case 0x83: printf("ADD E\n"); break;
    case 0x84: printf("ADD H\n"); break;
    case 0x85: printf("ADD L\n"); break;
    case 0x86: printf("ADD M\n"); break;
    case 0x87: printf("ADD A\n"); break;

    case 0x88: printf("ADC B\n"); break;
    case 0x89: printf("ADC C\n"); break;
    case 0x8A: printf("ADC D\n"); break;
    case 0x8B: printf("ADC E\n"); break;
    case 0x8C: printf("ADC H\n"); break;
    case 0x8D: printf("ADC L\n"); break;
    case 0x8E: printf("ADC M\n"); break;
    case 0x8F: printf("ADC A\n"); break;

    case 0x90: printf("SUB B\n"); break;
    case 0x91: printf("SUB C\n"); break;
    case 0x92: printf("SUB D\n"); break;
    case 0x93: printf("SUB E\n"); break;
    case 0x94: printf("SUB H\n"); break;
    case 0x95: printf("SUB L\n"); break;
    case 0x96: printf("SUB M\n"); break;
    case 0x97: printf("SUB A\n"); break;

    case 0x98: printf("SBB B\n"); break;
    case 0x99: printf("SBB C\n"); break;
    case 0x9A: printf("SBB D\n"); break;
    case 0x9B: printf("SBB E\n"); break;
    case 0x9C: printf("SBB H\n"); break;
    case 0x9D: printf("SBB L\n"); break;
    case 0x9E: printf("SBB M\n"); break;
    case 0x9F: printf("SBB A\n"); break;

    case 0xA0: printf("ANA B\n"); break;
    case 0xA1: printf("ANA C\n"); break;
    case 0xA2: printf("ANA D\n"); break;
    case 0xA3: printf("ANA E\n"); break;
    case 0xA4: printf("ANA H\n"); break;
    case 0xA5: printf("ANA L\n"); break;
    case 0xA6: printf("ANA M\n"); break;
    case 0xA7: printf("ANA A\n"); break;

    case 0xA8: printf("XRA B\n"); break;
    case 0xA9: printf("XRA C\n"); break;
    case 0xAA: printf("XRA D\n"); break;
    case 0xAB: printf("XRA E\n"); break;
    case 0xAC: printf("XRA H\n"); break;
    case 0xAD: printf("XRA L\n"); break;
    case 0xAE: printf("XRA M\n"); break;
    case 0xAF: printf("XRA A\n"); break;

    case 0xB0: printf("ORA B\n"); break;
    case 0xB1: printf("ORA C\n"); break;
    case 0xB2: printf("ORA D\n"); break;
    case 0xB3: printf("ORA E\n"); break;
    case 0xB4: printf("ORA H\n"); break;
    case 0xB5: printf("ORA L\n"); break;
    case 0xB6: printf("ORA M\n"); break;
    case 0xB7: printf("ORA A\n"); break;

    case 0xB8: printf("CMP B\n"); break;
    case 0xB9: printf("CMP C\n"); break;
    case 0xBA: printf("CMP D\n"); break;
    case 0xBB: printf("CMP E\n"); break;
    case 0xBC: printf("CMP H\n"); break;
    case 0xBD: printf("CMP L\n"); break;
    case 0xBE: printf("CMP M\n"); break;
    case 0xBF: printf("CMP A\n"); break;

    case 0xC0: printf("RNZ\n"); break;
    case 0xC1: printf("POP B\n"); break;
    case 0xC2: printf("JNZ 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xC3: printf("JMP 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xC4: printf("CNZ 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xC5: printf("PUSH B\n"); break;
    case 0xC6: printf("ADI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xC7: printf("RST 0\n"); break;
    case 0xC8: printf("RZ\n"); break;
    case 0xC9: printf("RET\n"); break;
    case 0xCA: printf("JZ 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    // 0xCB : invalid.
    case 0xCC: printf("CZ 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xCD: printf("CALL 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xCE: printf("ACI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xCF: printf("RST 1\n"); break;

    case 0xD0: printf("RNC\n"); break;
    case 0xD1: printf("POP D\n"); break;
    case 0xD2: printf("JNC 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xD3: printf("OUT 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xD4: printf("CNC 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xD5: printf("PUSH D\n"); break;
    case 0xD6: printf("SUI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xD7: printf("RST 2\n"); break;
    case 0xD8: printf("RC\n"); break;
    // 0xD9 : invalid.
    case 0xDA: printf("JC 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xDB: printf("IN 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xDC: printf("CC 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    // 0xDD : invalid.
    case 0xDE: printf("SBI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xDF: printf("RST 3\n"); break;

    case 0xE0: printf("RPO\n"); break;
    case 0xE1: printf("POP H\n"); break;
    case 0xE2: printf("JPO 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xE3: printf("XTHL\n"); break;
    case 0xE4: printf("CPO 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xE5: printf("PUSH H\n"); break;
    case 0xE6: printf("ANI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xE7: printf("RST 4\n"); break;
    case 0xE8: printf("RPE\n"); break;
    case 0xE9: printf("PCHIL\n"); break;
    case 0xEA: printf("JPE 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xEB: printf("XCHG\n"); break;
    case 0xEC: printf("CPE 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    // 0xED : invalid.
    case 0xEE: printf("XRI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xEF: printf("RST 5\n"); break;

    case 0xF0: printf("RP\n"); break;
    case 0xF1: printf("POP PSW\n"); break;
    case 0xF2: printf("JP 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xF3: printf("DI\n"); break;
    case 0xF4: printf("CP 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xF5: printf("PUSH PSW\n"); break;
    case 0xF6: printf("ORI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xF7: printf("RST 6\n"); break;
    case 0xF8: printf("RM\n"); break;
    case 0xF9: printf("SPHL\n"); break;
    case 0xFA: printf("JM 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    case 0xFB: printf("EI\n"); break;
    case 0xFC: printf("CM 0x%X%X\n", buff[pc + 2], buff[pc + 1]); size = 3; break;
    // 0xFD : invalid.
    case 0xFE: printf("CPI 0x%X\n", buff[pc + 1]); size = 2; break;
    case 0xFF: printf("RST 7\n"); break;

    default: break;
  }

  return size;
}

static bool dissassemble_rom(FILE *fp) {
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

  // Dissassembly of the entire rome.
  while (pc < fsize) {
    int size = dissassemble_instr(rom_buff, pc);
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

  assemble_rom(fp);

  fclose(fp);

  return 0;
}
