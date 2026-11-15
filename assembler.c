#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "assembler.h"
// #include "utils.h"

#define LABELS_ARR_BASE_SZ 10
#define ROM_BUFF_BASE_SZ 2048
#define PARSED_ITEMS_BASE_SZ 32
#define TOKENS_BASE_SZ 10

#define RED   "\033[31m"
#define RESET "\033[0m"

enum TokenKind {
  TK_NONE,
  TK_IDENT,
  TK_PUNCT,
  TK_NUM,
  TK_NEWLINE,
  TK_INVALID,
};

enum NumParseRes {
  NUM_OK,
  NUM_OVERFLOW,
  NUM_INVALID
};

enum SyntaxKind {
  SK_OPCODE,
  SK_COMMA,
  SK_REG,
  SK_D8,
  SK_D16,
  SK_A16,
  SK_END
};

enum OperandType {
  OP_VALUE,
  OP_UNRESOLVED
};

enum ParsedItemType {
  PARSED_NONE,
  PARSED_INSTR,
  PARSED_LABEL
};

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

struct Error {
  char *text;
  size_t start, end;
};

struct Token {
  enum TokenKind kind;
  char *text;
  uint64_t value;
  size_t start, end;
  size_t line;
};

struct Operand {
  enum OperandType type;
  union {
    uint16_t value;
    char *unresolved;
  };
};

struct ParsedInstr {
  enum EncType enc_t;
  uint8_t opcode;
  struct Operand reg1;
  struct Operand reg2;
  struct Operand number;
};

struct ParsedItem {
  enum ParsedItemType type;
  union {
    struct ParsedInstr instr;
    char *label;
  };
};

struct Label {
  char *text;
  uint16_t addr;
};

struct OpcodeRec {
  char *name;
  enum EncType enc_t;
  uint8_t opcode;
};

static enum SyntaxKind syntax_table[][5] = {
  [ENC_FIXED] = {SK_OPCODE, SK_END},
  [ENC_REG_REG] = {SK_OPCODE, SK_REG, SK_COMMA, SK_REG, SK_END},
  [ENC_REG_ID] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_ARITH] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_SP] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_BD] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_PSW] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_SP_IMM16] = {SK_OPCODE, SK_REG, SK_COMMA, SK_D16, SK_END},
  [ENC_REG_IMM8] = {SK_OPCODE, SK_REG, SK_COMMA, SK_D8, SK_END},
  [ENC_IMM8] = {SK_OPCODE, SK_D8, SK_END},
  [ENC_ADDR16] = {SK_OPCODE, SK_A16, SK_END},
  [ENC_RST] = {SK_OPCODE, SK_D8, SK_END},
};

static struct OpcodeRec first_token[] = {
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
  {NULL}
};

static struct Label *labels = NULL;
static size_t labels_len = 0;
static size_t labels_cap = 0;

static struct Token *tokens = NULL;
static size_t tokens_len = 0;
static size_t tokens_cap = 0;

static struct Error *errors = NULL;
static size_t errors_len = 0;
static size_t errors_cap = 0;

static struct ParsedItem *parsed_items = NULL;
static size_t parsed_items_len = 0;
static size_t parsed_items_cap = 0;

static int add_label(const char *text, uint16_t addr) {
  if (labels_len == labels_cap) {
    size_t new_cap = labels_cap == 0 ? LABELS_ARR_BASE_SZ : labels_cap * 2;

    struct Label *tmp = realloc(labels, new_cap * sizeof(*labels));
    if (tmp == NULL)
      return -1;

    labels = tmp;
    labels_cap = new_cap;
  }

  char *copy = strdup(text);
  if (copy == NULL)
    return -1;

  labels[labels_len].text = copy;
  labels[labels_len].addr = addr;
  labels_len++;

  return 0;
}

static int get_label(const char *text) {
  for (size_t i = 0; i < labels_len; i++) {
    if (strcmp(labels[i].text, text) == 0)
      return labels[i].addr;
  }

  return -1;
}

static int register_code(const char *reg, enum EncType enc_t) {
  if (!reg) return -1;

  int reg_len = strlen(reg);
  if (reg_len == 0 || reg_len > 3) return -1;

  char reg_upper[4] = {0};
  int i = 0;
  while (reg[i] && i < 3) {
    reg_upper[i] = toupper(reg[i]);
    i++;
  }
  reg_upper[i] = '\0';

  int n = -1;

  if (reg_len == 1) {
    switch (reg_upper[0]) {
      case 'B':
        n = 0; break;
      case 'C':
        n = 1; break;
      case 'D':
        n = 2; break;
      case 'E':
        n = 3; break;
      case 'H':
        n = 4; break;
      case 'L':
        n = 5; break;
      case 'M':
        n = 6; break;
      case 'A':
        n = 7; break;
      default: break;
    }
  }

  switch (enc_t) {
    case ENC_REG_REG:
    case ENC_REG_ID:
    case ENC_REG_ARITH:
    case ENC_REG_IMM8:
        return n;

    case ENC_REG_PAIR_BD: {
        if (n >= 0 && n <= 2 && n % 2 == 0)
          return n / 2;
        return -1;
    }

    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_SP_IMM16:
    case ENC_REG_PAIR_PSW: {
        if (n >= 0 && n <= 4 && n % 2 == 0) {
          return n / 2;
        } else {
          char *s = (enc_t == ENC_REG_PAIR_PSW)? "PSW": "SP";
          if (strcmp(reg_upper, s) == 0) return 3;
        }
        return -1;
    }

    default:
      return -1;
  }
}

static enum NumParseRes str_to_num(const char *str, uint64_t *num) {
  if (!str || !*str || !num)
    return NUM_INVALID;

  int base = 10;
  const char *p = str;

  if (p[0] == '0' && p[1]) {
    switch (tolower((unsigned char)p[1])) {
      case 'x': base = 16; break;
      case 'b': base = 2;  break;
      case 'o': base = 8;  break;
    }

    if (base != 10)
      p += 2;
  }

  if (!*p)
    return NUM_INVALID;

  errno = 0;
  char *end;

  unsigned long long value = strtoull(p, &end, base);

  if (errno == ERANGE || value > UINT64_MAX)
    return NUM_OVERFLOW;

  if (end == p || *end != '\0')
    return NUM_INVALID;

  *num = (uint64_t)value;
  return NUM_OK;
}

static enum TokenKind get_tokenkind(enum TokenKind last_token_kind, char curr_token) {
  if (isalpha(curr_token) || curr_token == '_') {
    if (last_token_kind == TK_NUM)
      return TK_NUM;
    return TK_IDENT;
  }

  else if (isdigit(curr_token)) {
    if (last_token_kind == TK_IDENT)
      return TK_IDENT;
    return TK_NUM;
  }

  if (curr_token == ',' || curr_token == ':') return TK_PUNCT;
  else if (isspace(curr_token)) return TK_NONE;

  return TK_INVALID;
}

static inline bool is_single_token(char c) {
  enum TokenKind tokenkind = get_tokenkind(TK_NONE, c);
  return tokenkind == TK_PUNCT || tokenkind == TK_INVALID;
}

// Returns a malloc'ed, null terminated string which much be free'd.
static char *substr(const char *str, size_t from, size_t to) {
  if (!str || from >= to) return NULL;

  size_t str_len = strlen(str);

  if (from >= str_len || to > str_len) return NULL;
  
  size_t substr_len = to - from;
  char *substr = malloc(sizeof(char) * substr_len + 1);
  if (!substr) return NULL;

  for (size_t i = 0; i < substr_len; i++)
    substr[i] = str[from + i];

  substr[substr_len] = '\0';

  return substr;
}

static bool tokens_add(struct Token token) {
  if (tokens_len >= tokens_cap) {
    size_t new_cap = (tokens == NULL)? TOKENS_BASE_SZ: tokens_cap * 2;
    struct Token *tmp = realloc(tokens, sizeof(struct Token) * new_cap);
    if (!tmp) return false;
    tokens = tmp;
    tokens_cap = new_cap;
  }

  tokens[tokens_len++] = token;

  return true;
}

static inline void tokens_free() {
  for (size_t i = 0; i < tokens_len; i++) {
    free(tokens[i].text);
  }
  free(tokens);
  tokens = NULL;
  tokens_cap = tokens_len = 0;
}

static void errors_add(struct Error error) {
  if (errors_len >= errors_cap) {
    size_t new_cap = (tokens == NULL)? TOKENS_BASE_SZ: tokens_cap * 2;
    struct Error *tmp = realloc(errors, sizeof(struct Error) * new_cap);
    if (!tmp) return;
    errors = tmp;
    errors_cap += new_cap;
  }

  errors[errors_len++] = error;
}

static void errors_print(void) {
  if (errors_len) {
    for (int j = 0; j < errors_len; j++) {
      printf("%s\n", errors[j].text);
    }
  }
}

static void errors_free(void) {
  free(errors);
  errors = NULL;
  errors_cap = errors_len = 0;
}

static void tokenize(const char *line, size_t lineno) {
  if (!line) return;

  size_t from = 0;
  enum TokenKind last_token_kind = TK_NONE;
  enum TokenKind curr_token_kind = TK_NONE;
  bool alive = true;

  bool line_tokenized = false;

  for (int i = 0 ; alive ; i++) {
    if (line[i] == '\0') {
      curr_token_kind = TK_NONE;
      alive = false;
    } else {
      curr_token_kind = get_tokenkind(last_token_kind, line[i]);
    }
    
    if (curr_token_kind != last_token_kind || is_single_token(line[i])) {
      if (last_token_kind != TK_NONE) {
        char *token_str = substr(line, from, i);
        if (!token_str) return;
        uint64_t value = 0;

        if (last_token_kind == TK_NUM) {
          switch (str_to_num(token_str, &value)) {
            case NUM_INVALID:
              errors_add((struct Error){"Invalid numeric literal", from, i});
              break;
            case NUM_OVERFLOW:
              errors_add((struct Error){"Number literal out of range", from, i});
              break;
            case NUM_OK:
              break;
          }
        } else if (last_token_kind == TK_INVALID) {
          errors_add((struct Error){"Unrecognised token", from, i});
        }

        if (!tokens_add((struct Token){last_token_kind, token_str, value, from, i, lineno})) return;
        line_tokenized = true;
      }
      from = i;
      if (line[i] == ';') break; // handle comments.
    }

    last_token_kind = curr_token_kind;
  }

  if (line_tokenized) tokens_add((struct Token){.kind = TK_NEWLINE});
}

static bool get_oprec(const char *mnemonic, struct OpcodeRec *oprec) {
  if (!mnemonic) return false;

  int mnemonic_len = strlen(mnemonic);
  if (mnemonic_len == 0 || mnemonic_len > 4) return false;

  char mnemonic_upper[5] = {0};
  int i = 0;
  while (mnemonic[i] && i < 4) {
    mnemonic_upper[i] = toupper(mnemonic[i]);    
    i++;
  }
  mnemonic_upper[i] = '\0';

  for (int i = 0; first_token[i].name != NULL; i++) {
    if (strcmp(mnemonic_upper, first_token[i].name) == 0) {
      *oprec = first_token[i];
      return true;
    }
  }

  return false;
}

static bool parsed_items_add(struct ParsedItem parsed_item) {
  if (parsed_items_len >= parsed_items_cap) {
    size_t new_cap = (parsed_items == NULL)? PARSED_ITEMS_BASE_SZ: parsed_items_cap * 2;
    struct ParsedItem *tmp = realloc(parsed_items, sizeof(struct ParsedItem) * new_cap);
    if (!tmp) return false;
    parsed_items = tmp;
    parsed_items_cap = new_cap;
  }

  parsed_items[parsed_items_len++] = parsed_item;

  return true;
}

static void parsed_items_free(void) {
  free(parsed_items);
  parsed_items = NULL;
  parsed_items_cap = parsed_items_len = 0;
}

static struct Token* tokens_get(bool consume, size_t n) {
  if (n == 0) return NULL;

  static size_t get_token_i = 0;
  struct Token *ret_token = NULL;

  size_t step = n - 1;
  if (get_token_i + step < tokens_len) {
    ret_token = &tokens[get_token_i + step];
    if (consume) {
      get_token_i += n;      
    }
  }

  return ret_token;
}

static inline struct Token *tokens_consume(size_t n) {
  return tokens_get(true, n);
}

static inline struct Token *tokens_peek(size_t n) {
  return tokens_get(false, n);
}

static bool parse_label(void) {
  struct Token *first_token = tokens_peek(1);
  struct Token *second_token = tokens_peek(2);

  if (first_token && first_token->kind == TK_IDENT) {
    if (second_token && second_token->kind == TK_PUNCT && second_token->text[0] == ':') {
      parsed_items_add((struct ParsedItem){.type = PARSED_LABEL, .label = first_token->text});
      tokens_consume(2);
    }
  }

  if (tokens_peek(1) && tokens_peek(1)->kind == TK_NEWLINE) {
    tokens_consume(1);    
    return false;
  }

  return true;
}

static void parse_instr(void) {
  struct OpcodeRec oprec;
  enum SyntaxKind *syntax_array = NULL;
  size_t syntax_arr_i = 0;
  struct ParsedInstr parsed_instr = {ENC_NONE};

  while (true) {
    struct Token *token = tokens_consume(1);
    if (!token) return;

    if (syntax_array == NULL) {
      if (token->kind == TK_IDENT && get_oprec(token->text, &oprec)) {
          syntax_array = syntax_table[oprec.enc_t];
        } else {
          errors_add((struct Error){"Expected a valid opcode mnemonic", token->start, token->end});
          goto fail;
        }
    }

    if (token->kind == TK_NEWLINE && syntax_array[syntax_arr_i] != SK_END) {
      errors_add((struct Error){"Incomplete instruction, expected more operands", 0, 0});
      goto fail;
    }

    switch (syntax_array[syntax_arr_i]) {
      case SK_OPCODE:
        parsed_instr.enc_t = oprec.enc_t;
        parsed_instr.opcode = oprec.opcode;
        break;

      case SK_COMMA:
        if (token->kind != TK_PUNCT) {
          errors_add((struct Error){"Expected \',\' between operands", token->start, token->end});
          goto fail;
        }
        break;

      case SK_D8:
        if (token->kind != TK_NUM || token->value > 0xFF) {
          errors_add((struct Error){"Expected a value within the range 0x00 - 0xFF (8-bit)", token->start, token->end});
          goto fail;
        }

        parsed_instr.number.type = OP_VALUE;
        parsed_instr.number.value = token->value;
        break;

      case SK_D16:
      case SK_A16:
        if (token->kind == TK_NUM) {
          if (token->value > 0xFFFF) {
            errors_add((struct Error){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", token->start, token->end});
            goto fail;
          }
          parsed_instr.number.type = OP_VALUE;
          parsed_instr.number.value = token->value;
        } else if (syntax_array[syntax_arr_i] == SK_A16 && token->kind == TK_IDENT) {
          parsed_instr.number.type = OP_UNRESOLVED;
          parsed_instr.number.unresolved = token->text;
        } else {
          errors_add((struct Error){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", token->start, token->end});
          goto fail;
        }
        break;

      case SK_REG: {
        int register_c = register_code(token->text, oprec.enc_t);
        if (register_c == -1) {
          errors_add((struct Error){"Expected a register", token->start, token->end});
          goto fail;
        }
        if (syntax_arr_i == 1) {
          parsed_instr.reg1.type = OP_VALUE;
          parsed_instr.reg1.value = register_c;
        }
        else {
          parsed_instr.reg2.type = OP_VALUE;
          parsed_instr.reg2.value = register_c;
        }
        break;
      }

      case SK_END:
        if (token->kind != TK_NEWLINE) {
          errors_add((struct Error){"Unexpected trailing tokens", 0, 0});
          goto fail;
        }
        parsed_items_add((struct ParsedItem){.type = PARSED_INSTR, .instr = parsed_instr});
        return;
    }

  syntax_arr_i++;
  }

  return; // Should never be reached.

fail:
  while (tokens_peek(1) && tokens_peek(1)->kind != TK_NEWLINE)
    tokens_consume(1); // Consume everything till newline.
  tokens_consume(1); // Consume newline.
}

static bool parse(void) {
  if (parse_label()) parse_instr();
  return tokens_peek(1) != NULL;
}

static uint8_t get_instr_size(enum EncType enc_t) {
  switch (enc_t) {
    case ENC_NONE:
      return 0;

    case ENC_RST:
    case ENC_FIXED:
    case ENC_REG_ID:
    case ENC_REG_REG:
    case ENC_REG_ARITH:
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      return 1;

    case ENC_IMM8:
    case ENC_REG_IMM8:
      return 2;

    case ENC_ADDR16:
    case ENC_REG_PAIR_SP_IMM16:
      return 3;
  }
}

static uint32_t assemble_instr(struct ParsedInstr parsed_instr) {
  uint32_t asmbld_instr = 0;

  enum EncType enc_t = parsed_instr.enc_t;
  uint8_t opcode = parsed_instr.opcode;

  uint8_t register1_c = parsed_instr.reg1.value;
  uint8_t register2_c = parsed_instr.reg2.value;

  uint16_t number = parsed_instr.number.value;

  switch (enc_t) {
    case ENC_NONE:
      break;

    case ENC_FIXED:
      asmbld_instr = opcode;
      break;

    case ENC_REG_REG:
      asmbld_instr = opcode | (register1_c << 3) | register2_c;
      break;

    case ENC_REG_ID:
      asmbld_instr = opcode | (register1_c << 3);
      break;

    case ENC_REG_ARITH:
      asmbld_instr = opcode | register1_c;
      break;

    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      asmbld_instr = opcode | (register1_c << 4);
      break;

    case ENC_REG_PAIR_SP_IMM16:
      asmbld_instr = number << 8 | (opcode + (register1_c << 4));
      break;

    case ENC_REG_IMM8:
      asmbld_instr = number << 8 | (opcode + (register1_c << 3));
      break;

    case ENC_IMM8:
      asmbld_instr = number << 8 | opcode;
      break;

    case ENC_ADDR16:
      asmbld_instr = number << 8 | opcode;
      break;

    case ENC_RST:
      asmbld_instr = opcode | (number << 3);
      break;
  }

  uint8_t size = get_instr_size(enc_t);
  return (size << 24) | asmbld_instr;
}

bool assemble_rom(char *read_from, char *write_to) {
  return true;
}

int main(void) {
  tokenize("", 2);
  tokenize("      ;sdfds", 3);
  tokenize("label1", 4);
  tokenize("label_2:", 6);
  tokenize("label_3:mov a, b", 7);
  tokenize("jmp label_2", 8);

  while (parse());

  for (int i = 0; i < parsed_items_len; i++) {
    char *s;
    switch (parsed_items[i].type) {
      case PARSED_NONE:
        s = "NONE";
        break;
      case PARSED_LABEL:
        s = "LABEL";
        break;
      case PARSED_INSTR:
        s = "INSTR";
        break;
    }

    printf("%s: ", s);
    if (parsed_items[i].type == PARSED_LABEL) {
      printf("%s\n", parsed_items[i].label);
    } else if (parsed_items[i].type == PARSED_INSTR) {
      printf("0x%x ", parsed_items[i].instr.opcode);
      if (parsed_items[i].instr.number.type == OP_UNRESOLVED)
        printf("%s\n", parsed_items[i].instr.number.unresolved);
      else
        printf("\n");
    } else {
      printf("\n");
    }
  }

  errors_print();

  tokens_free();
  errors_free();
  return 0;
}
