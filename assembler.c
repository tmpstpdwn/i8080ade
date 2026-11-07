#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include <stdio.h>

#include "assembler.h"

enum TokenKind {
  NONE,
  IDENTIFIER,
  PUNCTUATION,
  NUMBER,
  UNKNOWN,
};

struct Token {
  enum TokenKind kind;
  char *text;
  uint64_t value;
  size_t col;
};

struct TokenList {
  struct Token *tokens;
  size_t len, cap;
};

enum OpcodeType {
  OP_FIXED,
  OP_REG_REG,
  OP_REG_ID, // INR, DCR.
  OP_REG_ARITH, // Arithmetic.
  OP_REG_PAIR_SP,
  OP_REG_PAIR_BD,
  OP_REG_PAIR_PSW,
  OP_REG_PAIR_SP_IMM16,
  OP_REG_IMM8,
  OP_IMM8,
  OP_ADDR16,
  OP_RST
};

struct OpRec {
  char *name;
  enum OpcodeType type;
  uint8_t opcode;
};

static const char *kind_names[] = {
    "NONE", "IDENTIFIER", "PUNCTUATION", "NUMBER", "UNKNOWN"
};

static struct OpRec opcode_table[] = {
  {"NOP",       OP_FIXED,                  0x00},
  {"LXI",       OP_REG_PAIR_SP_IMM16,      0x01},
  {"STAX",      OP_REG_PAIR_BD,            0x02},
  {"INX",       OP_REG_PAIR_SP,            0x03},
  {"INR",       OP_REG_ID,                 0x04},
  {"DCR",       OP_REG_ID,                 0x05},
  {"MVI",       OP_REG_IMM8,               0x06},
  {"RLC",       OP_FIXED,                  0x07},
  {"DAD",       OP_REG_PAIR_SP,            0x09},
  {"LDAX",      OP_REG_PAIR_BD,            0x0A},
  {"DCX",       OP_REG_PAIR_SP,            0x0B},
  {"RRC",       OP_FIXED,                  0x0F},
  {"RAL",       OP_FIXED,                  0x17},
  {"RAR",       OP_FIXED,                  0x1F},
  {"SHLD",      OP_ADDR16,                 0x22},
  {"DAA",       OP_FIXED,                  0x27},
  {"LHLD",      OP_ADDR16,                 0x2A},
  {"CMA",       OP_FIXED,                  0x2F},
  {"STA",       OP_ADDR16,                 0x32},
  {"STC",       OP_FIXED,                  0x37},
  {"LDA",       OP_ADDR16,                 0x3a},
  {"CMC",       OP_FIXED,                  0x3F},
  {"MOV",       OP_REG_REG,                0x40},
  {"HLT",       OP_FIXED,                  0x76},
  {"ADD",       OP_REG_ARITH,              0x80},
  {"ADC",       OP_REG_ARITH,              0x88},
  {"SUB",       OP_REG_ARITH,              0x90},
  {"SBB",       OP_REG_ARITH,              0x98},
  {"ANA",       OP_REG_ARITH,              0xA0},
  {"XRA",       OP_REG_ARITH,              0xA8},
  {"ORA",       OP_REG_ARITH,              0xB0},
  {"CMP",       OP_REG_ARITH,              0xB8},
  {"RNZ",       OP_FIXED,                  0xC0},
  {"POP",       OP_REG_PAIR_PSW,           0xC1},
  {"JNZ",       OP_ADDR16,                 0xC2},
  {"JMP",       OP_ADDR16,                 0xC3},
  {"CNZ",       OP_ADDR16,                 0xC4},
  {"PUSH",      OP_REG_PAIR_PSW,           0xc5},
  {"ADI",       OP_IMM8,                   0xC6},
  {"RST",       OP_RST,                    0xC7},
  {"RZ",        OP_FIXED,                  0xC8},
  {"RET",       OP_FIXED,                  0xC9},
  {"JZ",        OP_ADDR16,                 0xCA},
  {"CZ",        OP_ADDR16,                 0xCC},
  {"CALL",      OP_ADDR16,                 0xCD},
  {"ACI",       OP_IMM8,                   0xCE},
  {"RNC",       OP_FIXED,                  0xD0},
  {"JNC",       OP_ADDR16,                 0xD2},
  {"OUT",       OP_IMM8,                   0xD3},
  {"CNC",       OP_ADDR16,                 0xD4},
  {"SUI",       OP_IMM8,                   0xD6},
  {"RC",        OP_FIXED,                  0xD8},
  {"JC",        OP_ADDR16,                 0xDA},
  {"IN",        OP_IMM8,                   0xDB},
  {"CC",        OP_ADDR16,                 0xDC},
  {"SBI",       OP_IMM8,                   0xDE},
  {"RPO",       OP_FIXED,                  0xE0},
  {"JPO",       OP_ADDR16,                 0xE2},
  {"XTHL",      OP_FIXED,                  0xE3},
  {"CPO",       OP_ADDR16,                 0xE4},
  {"ANI",       OP_IMM8,                   0xE6},
  {"RPE",       OP_FIXED,                  0xE8},
  {"PCHL",      OP_FIXED,                  0xE9},
  {"JPE",       OP_ADDR16,                 0xEA},
  {"XCHG",      OP_FIXED,                  0xEB},
  {"CPE",       OP_ADDR16,                 0xEC},
  {"XRI",       OP_IMM8,                   0xEE},
  {"RP",        OP_FIXED,                  0xF0},
  {"JP",        OP_ADDR16,                 0xF2},
  {"DI",        OP_FIXED,                  0xF3},
  {"CP",        OP_ADDR16,                 0xF4},
  {"ORI",       OP_IMM8,                   0xF6},
  {"RM",        OP_FIXED,                  0xF8},
  {"SPHL",      OP_FIXED,                  0xF9},
  {"JM",        OP_ADDR16,                 0xFA},
  {"EI",        OP_FIXED,                  0xFB},
  {"CM",        OP_ADDR16,                 0xFC},
  {"CPI",       OP_IMM8,                   0xFE},
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
    case OP_REG_ARITH:
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
    case OP_REG_PAIR_SP_IMM16:
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
  while (s && *s) {
    if (isalpha(*s)) {
      *s = toupper(*s);
    }
    s++;
  }
}

static bool str_to_hex(const char *str, uint64_t *num) {
  if (!str || !num) return false;

  char *endptr = NULL;
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

static enum TokenKind get_tokenkind(enum TokenKind last_token_kind, char last_token, char curr_token) {
  if (isalpha(curr_token)) {
    if (last_token == '0') {
      char c = toupper(curr_token);
      if (c == 'X' || c == 'B')
        return NUMBER;
    }
    return IDENTIFIER;
  }

  else if (isdigit(curr_token)) {
    if (last_token_kind == IDENTIFIER)
      return IDENTIFIER;
    return NUMBER;
  }

  if (curr_token == ',' || curr_token == ':') return PUNCTUATION;
  else if (curr_token == ' ') return NONE;

  return UNKNOWN;
}

static bool is_single_token(char c) {
  return get_tokenkind(NONE, ' ', c) == PUNCTUATION;
}

static char *substr(const char *str, size_t from, size_t to) {
  if (!str || from >= to) return NULL;

  size_t str_len = strlen(str);

  if (from >= str_len || to > str_len) return NULL;
  
  size_t substr_len = to - from;
  char *substr = malloc(sizeof(char) * substr_len + 1);
  if (!substr) return NULL;

  for (int i = 0; i < substr_len; i++)
    substr[i] = str[from + i];

  substr[substr_len] = '\0';

  return substr;
}

static bool add_token(struct TokenList *token_list, struct Token token) {
  if (token_list->len >= token_list->cap) {
    struct Token *tmp = realloc(token_list->tokens, sizeof(struct Token) * (token_list->cap + 5));
    if (!tmp) return false;
    token_list->tokens = tmp;
    token_list->cap += 5;
  }

  token_list->tokens[token_list->len++] = token;

  return true;
}

static struct TokenList get_token_list(const char *s) {
  struct TokenList token_list = {NULL, 0, 0};

  if (!s) goto fail;

  size_t from;
  char last_token = ' ';
  enum TokenKind last_token_kind = NONE;
  enum TokenKind curr_token_kind = NONE;
  bool alive = true;

  for (int i = 0 ; alive ; i++) {
    if (s[i] == '\0') {
      curr_token_kind = NONE;
      alive = false;
    } else {
      curr_token_kind = get_tokenkind(last_token_kind, last_token, s[i]);
    }
    
    if (curr_token_kind != last_token_kind || is_single_token(s[i])) {
      if (last_token_kind != NONE) {
        char *token = substr(s, from, i);
        if (!token || !add_token(&token_list, (struct Token){last_token_kind, token, 0, i})) goto fail;
      }
      from = i;
    }

    last_token_kind = curr_token_kind;
    last_token = s[i];
  }

  return token_list;

fail:
  printf("%d\n", (int)token_list.len);
  free(token_list.tokens);
  return (struct TokenList){0};
}

uint32_t assemble(const char *s) {
  if (!s) return 0;

  size_t len = strlen(s);

  if (len == 0) return 0;

  char *text_instr = malloc(len);
  if (!text_instr) return 0;
  strcpy(text_instr, s);

  char *token = strtok(text_instr, " ,");
  if (!token) return 0;

  uint32_t asmbld_instr = 0;

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
          asmbld_instr = opcode | (operand1_c << 3) | operand2_c;
          break;

        case OP_REG_ID:
          if (operand1_c == -1) break;
          asmbld_instr = opcode | (operand1_c << 3);
          break;

        case OP_REG_ARITH:
          if (operand1_c == -1) break;
          asmbld_instr = opcode | operand1_c;
          break;

        case OP_REG_PAIR_SP:
        case OP_REG_PAIR_BD:
        case OP_REG_PAIR_PSW:
          if (operand1_c == -1) break;
          asmbld_instr = opcode | (operand1_c << 4);
          break;

        case OP_REG_PAIR_SP_IMM16: {
          if (operand1_c == -1) break;

          uint64_t val;
          if (str_to_hex(operand2, &val) == false) break;
          if (val > UINT16_MAX) break;

          uint8_t final_opcode = opcode + (operand1_c << 4);

          asmbld_instr = val << 8 | final_opcode;
          break;
        }

        case OP_REG_IMM8: {
          if (operand1_c == -1) break;

          uint64_t val;
          if (str_to_hex(operand2, &val) == false) break;
          if (val > UINT8_MAX) break;

          uint8_t final_opcode = opcode + (operand1_c << 3);

          asmbld_instr = val << 8 | final_opcode;
          break;
        }

        case OP_IMM8: {
          uint64_t val;

          if (str_to_hex(operand1, &val) == false) break;
          if (val > UINT8_MAX) break;

          asmbld_instr = val << 8 | opcode;
          break;
        }

        case OP_ADDR16: {
          uint64_t val;

          if (str_to_hex(operand1, &val) == false) break;
          if (val > UINT16_MAX) break;

          asmbld_instr = val << 8 | opcode;
          break;
        }

        case OP_RST: {
          uint64_t val;

          if (str_to_hex(operand1, &val) == false) break;
          if (val >= 8) break;

          asmbld_instr = opcode | (val << 3);
          break;
        }
         
        default: break;
      }
    }
  }

  free(text_instr);
  return asmbld_instr;
}  

static inline void free_token_list(struct TokenList token_list) {
  free(token_list.tokens);
}

int main(void) {
  struct TokenList token_list = get_token_list("MAIN: lxi B, 0xBEEF;mvi a,0Xff  delay0b10,  'A' 0b 0x 0xGG  123456789012345678901234567890 ,,:::  CALL foo+1  JMP  MAIN___2   0xAB_CD pop psW,xra c!ret");
  for (int i = 0; i < token_list.len; i++) {
    printf("%s %s\n", token_list.tokens[i].text, kind_names[token_list.tokens[i].kind]);
  }
  free_token_list(token_list);
  return 0;
}
