#include <errno.h>
#include <ctype.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

#include <stdio.h>

#include "assembler.h"

#define RED   "\033[31m"
#define RESET "\033[0m"

enum TokenKind {
  NONE,
  IDENTIFIER,
  PUNCTUATION,
  NUMBER,
  INVALID,
};

struct Token {
  enum TokenKind kind;
  char *text;
  uint64_t value;
  size_t start, end;
};

struct TokenList {
  struct Token *tokens;
  size_t len, cap;
};

struct Error {
  char *text;
  size_t start, end;
};

struct ErrorList {
  struct Error *errors;
  char *line;
  size_t lineno;
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
    "NONE", "IDENTIFIER", "PUNCTUATION", "NUMBER", "INVALID"
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

static bool str_to_num(const char *str, uint64_t *num) {
    if (!str || !*str || !num)
        return false;

    int base = 10;
    const char *p = str;

    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        base = 16;
        p += 2;
    }
    else if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        base = 2;
        p += 2;
    }
    else if (p[0] == '0' && (p[1] == 'o' || p[1] == 'O')) {
        base = 8;
        p += 2;
    }

    if (!*p)
        return false;;

    if (base == 2) {
        uint64_t value = 0;

        for (; *p; p++) {
            if (*p != '0' && *p != '1')
                return false;

            unsigned digit = *p - '0';

            if (value > (UINT64_MAX - digit) / 2)
                return false;

            value = value * 2 + digit;
        }

        *num = value;
        return true;
    }

    errno = 0;
    char *end;

    unsigned long long value = strtoull(p, &end, base);

    if (errno == ERANGE || *end != '\0' || end == p)
      return false;

    *num = (uint64_t)value;
    return true;
}

static enum TokenKind get_tokenkind(enum TokenKind last_token_kind, char last_token, char curr_token) {
  if (isalpha(curr_token) || curr_token == '_') {
    if (last_token_kind == NUMBER)
      return NUMBER;
    return IDENTIFIER;
  }

  else if (isdigit(curr_token)) {
    if (last_token_kind == IDENTIFIER)
      return IDENTIFIER;
    return NUMBER;
  }

  if (curr_token == ',' || curr_token == ':') return PUNCTUATION;
  else if (curr_token == ' ') return NONE;

  return INVALID;
}

static bool is_single_token(char c) {
  enum TokenKind tokenkind = get_tokenkind(NONE, ' ', c);
  return tokenkind == PUNCTUATION || tokenkind == INVALID;
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

static bool token_list_add(struct TokenList *token_list, struct Token token) {
  if (token_list->len >= token_list->cap) {
    struct Token *tmp = realloc(token_list->tokens, sizeof(struct Token) * (token_list->cap + 5));
    if (!tmp) return false;
    token_list->tokens = tmp;
    token_list->cap += 5;
  }

  token_list->tokens[token_list->len++] = token;

  return true;
}

static inline void token_list_free(struct TokenList token_list) {
  for (int i = 0; i < token_list.len; i++) {
    free(token_list.tokens[i].text);
  }
  free(token_list.tokens);
}

static struct ErrorList error_list_new(const char *line, size_t lineno) {
  return (struct ErrorList){NULL, substr(line, 0, strlen(line)), lineno, 0, 0};
}

static void error_list_add(struct ErrorList *error_list, const char *text, size_t start, size_t end) {
  if (!error_list || !text) return;

  if (error_list->len >= error_list->cap) {
    struct Error *tmp = realloc(error_list->errors, sizeof(struct Error) * (error_list->cap + 5));
    if (!tmp) return;
    error_list->errors = tmp;
    error_list->cap += 5;
  }

  char *text_s = substr(text, 0, strlen(text));

  error_list->errors[error_list->len++] = (struct Error){text_s, start, end};
}

static void error_list_print(struct ErrorList error_list) {
    if (!error_list.len) return;

    for (size_t i = 0; i < error_list.len; i++) {
        const struct Error *e = &error_list.errors[i];
        printf("%zu:%zu: %serror%s: %s\n", error_list.lineno, e->start + 1, RED, RESET, e->text);

        printf("%4zu | ", error_list.lineno);
        printf("%.*s", (int)e->start, error_list.line);

        printf(RED);
        printf("%.*s", (int)(e->end - e->start),
               error_list.line + e->start);
        printf(RESET);

        printf("%s\n", error_list.line + e->end);

        printf("     | %*s", (int)e->start, "");

        // The "^~~~" in red, pin pointing the error.
        printf(RED);
        printf("^");
        size_t len = e->end - e->start - 1;
        for (int i = 0; i < len; i++) {
          printf("~");
        }
        printf(RESET);
        printf("\n");
    }
}

static void error_list_free(struct ErrorList error_list) {
  for (int i = 0; i < error_list.len; i++) {
    free(error_list.errors[i].text);
  }
  free(error_list.errors);
}

static struct TokenList tokenize(const char *s, struct ErrorList *error_list) {
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
        char *token_str = substr(s, from, i);
        if (!token_str) goto fail;
        size_t token_col = from;
        struct Token token = {last_token_kind, token_str, 0, from, i};

        if (last_token_kind == NUMBER) {
          if (str_to_num(token_str, &token.value)) {
            if (token.value > 0xFFFF) 
              error_list_add(error_list, "Number literal out of the 16-bit range", from, i);
          } else {
              error_list_add(error_list, "Invalid Number Literal", from, i);
          }
        } else if (last_token_kind == INVALID) {
          error_list_add(error_list, "Invalid token found", from, i);
        }

        if (!token_list_add(&token_list, token)) goto fail;
      }
      from = i;
    }

    last_token_kind = curr_token_kind;
    last_token = s[i];
  }

  return token_list;

fail:
  free(token_list.tokens);
  return (struct TokenList){0};
}

static void parse(const struct TokenList *token_list, struct ErrorList *error_list) {
  return;
}

static void check_semantics(const struct TokenList *token_list, struct ErrorList *error_list) {
  return;
}

static uint32_t assemble_instr(const struct TokenList *token_list) {
  uint32_t asmbld_instr = 0;

  for (int i = 0; opcode_table[i].name != NULL; i++) {
    if (strcmp(token_list->tokens[0].text, opcode_table[i].name) == 0) {
      enum OpcodeType op_t = opcode_table[i].type;
      uint8_t opcode = opcode_table[i].opcode;

      int operand1_c = -1;
      int operand2_c = -1;
      uint64_t val = 0;

      switch (token_list->len) {
        case 4:
          operand2_c = operand_code(token_list->tokens[3].text, op_t);
          val = token_list->tokens[3].value;
        case 2:
          operand1_c = operand_code(token_list->tokens[1].text, op_t);
          if (!val) val = token_list->tokens[1].value;
        default:
          break;
      }

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

        case OP_REG_PAIR_SP_IMM16:
          if (operand1_c == -1) break;
          asmbld_instr = val << 8 | (opcode + (operand1_c << 4));
          break;

        case OP_REG_IMM8:
          if (operand1_c == -1) break;
          asmbld_instr = val << 8 | (opcode + (operand1_c << 3));
          break;

        case OP_IMM8:
          asmbld_instr = val << 8 | opcode;
          break;

        case OP_ADDR16:
          asmbld_instr = val << 8 | opcode;
          break;

        case OP_RST:
          asmbld_instr = opcode | (val << 3);
          break;
         
        default: break;
      }

      break;
    }
  }

  return asmbld_instr;
}

uint32_t assemble(const char *line, size_t lineno) {
  struct ErrorList error_list = error_list_new(line, lineno);
  struct TokenList token_list = {0};

  uint32_t assembled_instr = 0;

  token_list = tokenize(line, &error_list);
  if (error_list.errors) goto errors;
  parse(&token_list, &error_list);
  if (error_list.errors) goto errors;
  check_semantics(&token_list, &error_list);
  if (error_list.errors) goto errors;
  assembled_instr = assemble_instr(&token_list);

errors:
  error_list_print(error_list);
  error_list_free(error_list);
  token_list_free(token_list);
  
  return assembled_instr;
}
