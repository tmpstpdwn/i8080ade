#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// #include "assembler.h"

#include "vector.h"

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

struct SourceFile {
  size_t id;
  char *text;
  const char *name;
  struct Vector *line_offsets;
  struct Vector *labels;
  struct Vector* tokens;
  struct Vector* diagnostics;
  struct Vector* parsed_items;
};

struct SourceManager {
  struct Vector *source_files;
};

struct Diagnostic {
  char *text;
  size_t start, end;
};

struct Token {
  enum TokenKind kind;
  char *text;
  uint64_t value;
  size_t start, end;
};

struct Operand {
  enum OperandType type;
  union {
    uint16_t value;
    char *unresolved;
  };
};

struct ParsedLabel {
  char *text;
};

struct ParsedInstr {
  enum EncType enc_t;
  uint8_t opcode;
  uint8_t reg1;
  uint8_t reg2;
  struct Operand number;
};

struct ParsedItem {
  enum ParsedItemType type;
  union {
    struct ParsedInstr instr;
    struct ParsedLabel label;
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

static bool labels_add(struct SourceFile *source_file, struct Label label) {
  if (!source_file) return false;
  return vector_append(source_file->labels, &label);
}

static bool labels_resolve(struct SourceFile *source_file, const char *text, uint16_t *resolved_val) {
  if (!text || !resolved_val) return false;

  size_t label_table_len = vector_len(source_file->labels);

  for (size_t i = 0; i < label_table_len; i++) {
    struct Label *label = vector_get(source_file->labels, i);
    if (strcmp(label->text, text) == 0) {
      *resolved_val = label->addr;
      return true;
    }
  }

  return false;
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
  else if (curr_token == '\n') return TK_NEWLINE;
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

static void diagnostics_print(void) {
  // WIP
}

static bool diagnostics_add(struct SourceFile *source_file, struct Diagnostic diagnostic) {
  if (!source_file) return false;
  return vector_append(source_file->diagnostics, &diagnostic);
}

static bool tokens_add(struct SourceFile *source_file, struct Token token) {
  if (!source_file) return false;
  return vector_append(source_file->tokens, &token);
}

static bool lexer_tokenize(struct SourceFile *source_file) {
  if (!source_file) return false;

  size_t from = 0;
  enum TokenKind last_token_kind = TK_NONE;
  enum TokenKind curr_token_kind = TK_NONE;
  bool alive = true;

  char *file_text = source_file->text;

  for (int i = 0 ; alive ; i++) {
    if (file_text[i] == '\0') {
      curr_token_kind = TK_NONE;
      alive = false;
    } else {
      curr_token_kind = get_tokenkind(last_token_kind, file_text[i]);
    }
    
    if (curr_token_kind != last_token_kind || is_single_token(file_text[i])) {
      if (last_token_kind != TK_NONE) {
        char *token_str = substr(file_text, from, i);
        if (!token_str) return false;
        uint64_t value = 0;

        if (last_token_kind == TK_NUM) {
          switch (str_to_num(token_str, &value)) {
            case NUM_INVALID:
              diagnostics_add(source_file, (struct Diagnostic){"Invalid numeric literal", from, i});
              break;
            case NUM_OVERFLOW:
              diagnostics_add(source_file, (struct Diagnostic){"Number literal out of range", from, i});
              break;
            case NUM_OK:
              break;
          }
        } else if (last_token_kind == TK_INVALID) {
          diagnostics_add(source_file, (struct Diagnostic){"Unrecognised token", from, i});
        }

        if (!tokens_add(source_file, (struct Token){last_token_kind, token_str, value, from, i})) return false;
      }
      from = i;
    }

    last_token_kind = curr_token_kind;
  }

  return true;
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

static bool parsed_items_add(struct SourceFile *source_file, struct ParsedItem parsed_item) {
  if (!source_file) return false;
  return vector_append(source_file->parsed_items, &parsed_item);
}

static struct Token* tokens_get(struct SourceFile *source_file, bool consume, size_t n) {
  if (!source_file || n == 0) return NULL;

  static size_t get_token_i = 0;
  struct Token *ret_token = NULL;

  size_t step = n - 1;
  size_t tokens_len = vector_len(source_file->tokens);
  if (get_token_i + step < tokens_len) {
    ret_token = vector_get(source_file->tokens, get_token_i + step);
    if (consume) {
      get_token_i += n;      
    }
  }

  return ret_token;
}

static inline struct Token *tokens_consume(struct SourceFile *source_file, size_t n) {
  return tokens_get(source_file, true, n);
}

static inline struct Token *tokens_peek(struct SourceFile *source_file, size_t n) {
  return tokens_get(source_file, false, n);
}

static bool parse_label(struct SourceFile *source_file) {
  if (!source_file) return false;

  struct Token *first_token = tokens_peek(source_file, 1);
  struct Token *second_token = tokens_peek(source_file, 2);

  if (first_token && first_token->kind == TK_IDENT) {
    if (second_token && second_token->kind == TK_PUNCT && second_token->text[0] == ':') {
      struct ParsedLabel label = {first_token->text};
      parsed_items_add(source_file, (struct ParsedItem){.type = PARSED_LABEL, .label = label});
      tokens_consume(source_file, 2);
    }
  }

  if (tokens_peek(source_file, 1) && tokens_peek(source_file, 1)->kind == TK_NEWLINE) {
    tokens_consume(source_file, 1);    
    return false;
  }

  return true;
}

static void parse_instr(struct SourceFile *source_file) {
  if (!source_file) return;

  struct OpcodeRec oprec;
  enum SyntaxKind *syntax_array = NULL;
  size_t syntax_arr_i = 0;
  struct ParsedInstr parsed_instr = {ENC_NONE};

  while (true) {
    struct Token *token = tokens_consume(source_file, 1);
    if (!token) return;

    if (syntax_array == NULL) {
      if (token->kind == TK_IDENT && get_oprec(token->text, &oprec)) {
          syntax_array = syntax_table[oprec.enc_t];
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a valid opcode mnemonic", token->start, token->end});
          goto fail;
        }
    }

    if (token->kind == TK_NEWLINE && syntax_array[syntax_arr_i] != SK_END) {
      diagnostics_add(source_file, (struct Diagnostic){"Incomplete instruction, expected more operands", 0, 0});
      goto fail;
    }

    switch (syntax_array[syntax_arr_i]) {
      case SK_OPCODE:
        parsed_instr.enc_t = oprec.enc_t;
        parsed_instr.opcode = oprec.opcode;
        break;

      case SK_COMMA:
        if (token->kind != TK_PUNCT) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected \',\' between operands", token->start, token->end});
          goto fail;
        }
        break;

      case SK_D8:
        if (token->kind != TK_NUM || token->value > 0xFF) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x00 - 0xFF (8-bit)", token->start, token->end});
          goto fail;
        }

        parsed_instr.number.type = OP_VALUE;
        parsed_instr.number.value = token->value;
        break;

      case SK_D16:
      case SK_A16:
        if (token->kind == TK_NUM) {
          if (token->value > 0xFFFF) {
            diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", token->start, token->end});
            goto fail;
          }
          parsed_instr.number.type = OP_VALUE;
          parsed_instr.number.value = token->value;
        } else if (syntax_array[syntax_arr_i] == SK_A16 && token->kind == TK_IDENT) {
          parsed_instr.number.type = OP_UNRESOLVED;
          parsed_instr.number.unresolved = token->text;
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", token->start, token->end});
          goto fail;
        }
        break;

      case SK_REG: {
        int register_c = register_code(token->text, oprec.enc_t);
        if (register_c == -1) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a register", token->start, token->end});
          goto fail;
        }
        if (syntax_arr_i == 1) {
          parsed_instr.reg1 = register_c;
        }
        else {
          parsed_instr.reg2 = register_c;
        }
        break;
      }

      case SK_END:
        if (token->kind != TK_NEWLINE) {
          diagnostics_add(source_file, (struct Diagnostic){"Unexpected trailing tokens", 0, 0});
          goto fail;
        }
        parsed_items_add(source_file, (struct ParsedItem){.type = PARSED_INSTR, .instr = parsed_instr});
        return;
    }

  syntax_arr_i++;
  }

  return; // Should never be reached.

fail:
  while (tokens_peek(source_file, 1) && tokens_peek(source_file, 1)->kind != TK_NEWLINE)
    tokens_consume(source_file, 1); // Consume everything till newline.
  tokens_consume(source_file, 1); // Consume newline.
}

static void parser_parse(struct SourceFile *source_file) {
  while (tokens_peek(source_file, 1)) {
    if (!parse_label(source_file)) continue;
    parse_instr(source_file);
  } 
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

  uint8_t register1_c = parsed_instr.reg1;
  uint8_t register2_c = parsed_instr.reg2;

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

static void build_label_table(struct SourceFile *source_file) {
  uint16_t address_counter = 0;

  size_t parsed_items_len = vector_len(source_file->parsed_items);
  for (int i = 0; i < parsed_items_len; i++) {
    struct ParsedItem *p = vector_get(source_file->parsed_items, i);
    switch (p->type) {
      case PARSED_NONE:
        break;
      case PARSED_LABEL:
        labels_add(source_file, (struct Label){p->label.text, address_counter});
        break;
      case PARSED_INSTR:
        address_counter += get_instr_size(p->instr.enc_t);
        break;
    }
  }
}

static void resolve_labels(struct SourceFile *source_file) {
  size_t parsed_items_len = vector_len(source_file->parsed_items);
  for (int i = 0; i < parsed_items_len; i++) {
    struct ParsedItem *p = vector_get(source_file->parsed_items, i);
    if (p->type == PARSED_INSTR) {
      if (p->instr.number.type == OP_UNRESOLVED) {
        uint16_t resolved_val = 0;
        if (labels_resolve(source_file, p->instr.number.unresolved, &resolved_val)) {
          p->instr.number.type = OP_VALUE;
          p->instr.number.value = resolved_val;
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Undefined Symbol", 0, 0});
        }
      }
    }
  }
}

struct SourceManager source_manager_new(void) {
  struct SourceManager source_manager;
  source_manager.source_files = vector_new(sizeof(struct SourceFile));
  return source_manager;
}

static void source_manager_free(struct SourceManager source_manager) {
  size_t source_files_count = vector_len(source_manager.source_files);
  for (size_t i = 0; i < source_files_count; i++) {
    struct SourceFile *source_file = vector_get(source_manager.source_files, i); 
    vector_free(source_file->line_offsets);
    vector_free(source_file->diagnostics);
    vector_free(source_file->labels);
    vector_free(source_file->tokens);
    vector_free(source_file->parsed_items);
    free(source_file->text);
  }
  vector_free(source_manager.source_files);
}

static int source_manager_add_file(struct SourceManager *source_manager, char *text, const char *file_name) {
  if (!source_manager || !text || !file_name) return -1;

  struct Vector *line_offsets = vector_new(sizeof(size_t));
  struct Vector *labels = vector_new(sizeof(struct Label));
  struct Vector *errors = vector_new(sizeof(struct Diagnostic));
  struct Vector *tokens = vector_new(sizeof(struct Token));
  struct Vector *parsed_items = vector_new(sizeof(struct ParsedItem));

  if (!line_offsets || !errors || !tokens || !parsed_items) goto fail;

  for (size_t i = 0, l = 0; text[i]; i++) {
    if (text[i] == '\n') {
      if (!vector_append(line_offsets, &l)) goto fail;
      l = i + 1;
    }
  }

  size_t file_id = vector_len(source_manager->source_files);

  struct SourceFile source_file = {
    .id = file_id,
    .text = text,
    .name = file_name,
    .line_offsets = line_offsets,
    .labels = labels,
    .diagnostics = errors,
    .tokens = tokens,
    .parsed_items = parsed_items
   };

  if (!vector_append(source_manager->source_files, &source_file)) goto fail;

  return file_id;

fail:
  vector_free(line_offsets);
  vector_free(labels);
  vector_free(errors);
  vector_free(tokens);
  vector_free(parsed_items);
  return -1;
}

static size_t get_file_size(FILE *fp) {
  size_t file_size = 0;
  if (fp) {
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
  }
  return file_size;
}

static char *get_file_text(const char *read_from) {
  FILE *fp = fopen(read_from, "rb");
  char *text = NULL;

  if (!fp) return NULL;

  size_t file_size = get_file_size(fp);
  if (!file_size) goto end;

  text = malloc((size_t)file_size + 1);

  if (!text) goto end;

  size_t bytes_read = fread(text, 1, (size_t)file_size, fp);
  text[bytes_read] = '\0';

end:
  fclose(fp);
  return text;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    printf("expected a file\n");
    return 1;
  }
  const char *file_name = argv[1];

  char *file_text = get_file_text(file_name);
  if (!file_text) {
    printf("Reading \'%s\' into a buffer failed", file_name);
    return 1;
  }

  struct SourceManager source_manager = source_manager_new();
  int file_id = source_manager_add_file(&source_manager, file_text, file_name);
  struct SourceFile *source_file = vector_get(source_manager.source_files, file_id);

  lexer_tokenize(source_file);
  parser_parse(source_file);

  source_manager_free(source_manager);
  return 0;
}
