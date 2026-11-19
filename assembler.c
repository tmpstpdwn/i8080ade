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

struct LineOffset {
  size_t offset;
  size_t len;
};

struct SourceFile {
  char *text;
  const char *name;
  struct Vector *line_offsets;
  struct Vector *labels;
  struct Vector* tokens;
  struct Vector* diagnostics;
  struct Vector* parsed_items;
  struct Vector* buffer;
};

struct SourceRange {
  size_t lineno;
  size_t start_offset;
  size_t end_offset;
};

struct Diagnostic {
  char *text;
  struct SourceRange range;
};

struct Token {
  enum TokenKind kind;
  char *text;
  uint64_t value;
  struct SourceRange range;
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
    struct Label *label = vector_get_elem(source_file->labels, i);
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

static enum TokenKind lexer_get_tk(enum TokenKind last_token_kind, char curr_token) {
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

  if (curr_token == ',' || curr_token == ':' || curr_token == ';') return TK_PUNCT;
  else if (curr_token == '\n') return TK_NEWLINE;
  else if (isspace(curr_token) || curr_token == '\0') return TK_NONE;

  return TK_INVALID;
}

static inline bool is_single_token(char c) {
  enum TokenKind tokenkind = lexer_get_tk(TK_NONE, c);
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

static void diagnostics_print(struct SourceFile *source_file) {
  if (!source_file) return;

  for (size_t i = 0; i < vector_len(source_file->diagnostics); i++) {
    struct Diagnostic *diag = vector_get_elem(source_file->diagnostics, i);

    size_t line = diag->range.lineno;

    struct LineOffset *line_offset = vector_get_elem(source_file->line_offsets, line);

    size_t start = diag->range.start_offset;
    size_t end = diag->range.end_offset;

    size_t offset = line_offset->offset;
    size_t len = line_offset->len;

    if (start < offset || start > offset + len)
      continue;

    size_t column = start - offset;

    if (end > offset + len)
      end = offset + len;

    size_t highlight_len = end > start ? end - start : 1;

    fprintf(stderr, "%s:%zu:%zu: %serror:%s %s\n",
            source_file->name, line, column + 1,
            RED, RESET, diag->text);

    fprintf(stderr, " %4zu | %.*s%s%.*s%s%.*s\n",
            line,
            (int)column, source_file->text + offset,
            RED,
            (int)highlight_len, source_file->text + start,
            RESET,
            (int)(len - column - highlight_len),
            source_file->text + start + highlight_len);

    fprintf(stderr, "      | %*s%s^", (int)column, "", RED);

    for (size_t j = 1; j < highlight_len; j++)
      fputc('~', stderr);

    fprintf(stderr, "%s\n", RESET);
  }
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

  char *file_text = source_file->text;

  enum TokenKind last_tk = TK_NONE;
  enum TokenKind curr_tk = TK_NONE;

  size_t i = 0;
  size_t from = 0;
  size_t lineno = 0;

  while (true) {
    char c = file_text[i];
    curr_tk = lexer_get_tk(last_tk, c);
    
    // Process tokens.
    if (curr_tk != last_tk || is_single_token(c)) {
      if (last_tk != TK_NONE) {
        char *token_str = substr(file_text, from, i);
        if (!token_str) return false;
        uint64_t value = 0;
        struct SourceRange range = {lineno, from, i};

        if (last_tk == TK_NUM) {
          switch (str_to_num(token_str, &value)) {
            case NUM_INVALID:
              diagnostics_add(source_file, (struct Diagnostic){"Invalid numeric literal", range});
              last_tk = TK_INVALID;
              break;
            case NUM_OVERFLOW:
              diagnostics_add(source_file, (struct Diagnostic){"Number literal out of range", range});
              last_tk = TK_INVALID;
              break;
            case NUM_OK:
              break;
          }
        } else if (last_tk == TK_INVALID) {
          diagnostics_add(source_file, (struct Diagnostic){"Unrecognised token", range});
        } else if (last_tk == TK_NEWLINE) {
          lineno++;
          if (vector_len(source_file->tokens) == 0) {
            free(token_str);
            goto skip_token_add;
          }
        } else if (last_tk == TK_PUNCT) {
          // Skip comments.
          if (strcmp(token_str, ";") == 0) {
            while ((c = file_text[i++]) && c != '\n');
            if (c == '\n') i -= 2;
            curr_tk = TK_NONE;
            free(token_str);
            goto loop_end;
          }
        } 

        if (!tokens_add(source_file, (struct Token){last_tk, token_str, value, range})) {
          return false;
        }
      }

skip_token_add:
      from = i;
    }

loop_end:
    last_tk = curr_tk;
    i++;

    if (c == '\0') break;
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
    ret_token = vector_get_elem(source_file->tokens, get_token_i + step);
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

static bool parser_parse_line(struct SourceFile *source_file) {
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
    return true;
  }

  struct OpcodeRec oprec;
  enum SyntaxKind *syntax_array = NULL;
  size_t syntax_arr_i = 0;
  struct ParsedInstr parsed_instr = {ENC_NONE};

  while (true) {
    struct Token *token = tokens_consume(source_file, 1);
    if (!token) return false;

    if (token->kind == TK_INVALID) goto fail;

    if (syntax_array == NULL) {
      if (token->kind == TK_IDENT && get_oprec(token->text, &oprec)) {
          syntax_array = syntax_table[oprec.enc_t];
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a valid opcode mnemonic", token->range});
          goto fail;
        }
    }

    if (token->kind == TK_NEWLINE && syntax_array[syntax_arr_i] != SK_END) {
      diagnostics_add(source_file, (struct Diagnostic){"Incomplete instruction, expected more operands", token->range});
      goto fail;
    }

    switch (syntax_array[syntax_arr_i]) {
      case SK_OPCODE:
        parsed_instr.enc_t = oprec.enc_t;
        parsed_instr.opcode = oprec.opcode;
        break;

      case SK_COMMA:
        if (token->kind != TK_PUNCT) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected \',\' between operands", token->range});
          goto fail;
        }
        break;

      case SK_D8:
        if (token->kind != TK_NUM || token->value > 0xFF) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x00 - 0xFF (8-bit)", token->range});
          goto fail;
        }

        parsed_instr.number.type = OP_VALUE;
        parsed_instr.number.value = token->value;
        break;

      case SK_D16:
      case SK_A16:
        if (token->kind == TK_NUM) {
          if (token->value > 0xFFFF) {
            diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", token->range});
            goto fail;
          }
          parsed_instr.number.type = OP_VALUE;
          parsed_instr.number.value = token->value;
        } else if (syntax_array[syntax_arr_i] == SK_A16 && token->kind == TK_IDENT) {
          parsed_instr.number.type = OP_UNRESOLVED;
          parsed_instr.number.unresolved = token->text;
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", token->range});
          goto fail;
        }
        break;

      case SK_REG: {
        int register_c = register_code(token->text, oprec.enc_t);
        if (register_c == -1) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a register", token->range});
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
          diagnostics_add(source_file, (struct Diagnostic){"Unexpected trailing tokens", token->range});
          goto fail;
        }
        parsed_items_add(source_file, (struct ParsedItem){.type = PARSED_INSTR, .instr = parsed_instr});
        return true;
    }

  syntax_arr_i++;
  }

  return true;

fail:
  while (tokens_peek(source_file, 1) && tokens_peek(source_file, 1)->kind != TK_NEWLINE)
    tokens_consume(source_file, 1); // Consume everything till newline.
  tokens_consume(source_file, 1); // Consume newline.
  return true;
}

static void parser_parse(struct SourceFile *source_file) {
  while (parser_parse_line(source_file));
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

static uint32_t assemble_instr(struct ParsedInstr *parsed_instr) {
  uint32_t asmbld_instr = 0;

  enum EncType enc_t = parsed_instr->enc_t;
  uint8_t opcode = parsed_instr->opcode;

  uint8_t register1_c = parsed_instr->reg1;
  uint8_t register2_c = parsed_instr->reg2;

  uint16_t number = parsed_instr->number.value;

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

  return asmbld_instr;
}

static bool assemble(struct SourceFile *source_file) {
  for (int i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);

    switch (p->type) {
      case PARSED_INSTR: {
        uint8_t size = get_instr_size(p->instr.enc_t);
        uint32_t assembled_instr = assemble_instr(&p->instr);
        while (size) {
          uint8_t b = assembled_instr & 0xFF; 
          if (!vector_append(source_file->buffer, &b)) return false;
          assembled_instr = assembled_instr >> 8;
          size--;
        }
        break;
      }
      default:
        break;
    }

  }
  return true;
}

static void labels_build_table(struct SourceFile *source_file) {
  uint16_t address_counter = 0;

  size_t parsed_items_len = vector_len(source_file->parsed_items);
  for (int i = 0; i < parsed_items_len; i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    uint16_t dummy;
    switch (p->type) {
      case PARSED_NONE:
        break;
      case PARSED_LABEL:
        if (labels_resolve(source_file, p->label.text, &dummy)) {
          diagnostics_add(source_file, (struct Diagnostic){"Duplicate label", 0});
        } else {
          labels_add(source_file, (struct Label){p->label.text, address_counter});
        }
        break;
      case PARSED_INSTR:
        address_counter += get_instr_size(p->instr.enc_t);
        break;
    }
  }
}

static void labels_resolve_all(struct SourceFile *source_file) {
  size_t parsed_items_len = vector_len(source_file->parsed_items);
  for (int i = 0; i < parsed_items_len; i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
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

static struct SourceFile *source_file_new(const char *file_name, char *file_text) {
  if (!file_text || !file_name) return NULL;

  struct SourceFile *source_file = malloc(sizeof(struct SourceFile));

  struct Vector *line_offsets = vector_new(sizeof(struct LineOffset));
  struct Vector *labels = vector_new(sizeof(struct Label));
  struct Vector *errors = vector_new(sizeof(struct Diagnostic));
  struct Vector *tokens = vector_new(sizeof(struct Token));
  struct Vector *parsed_items = vector_new(sizeof(struct ParsedItem));
  struct Vector *buffer = vector_new(sizeof(uint8_t));

  if (!source_file || !line_offsets || !errors ||
      !tokens || !parsed_items || !buffer) goto fail;

  for (size_t i = 0, l = 0; ; i++) {
    if (file_text[i] == '\n' || file_text[i] == '\0') {

      struct LineOffset lo = {
        .offset = l,
        .len = i - l
      };

      if (!vector_append(line_offsets, &lo)) goto fail;
      l = i + 1;

      if (file_text[i] == '\0') break;
    }
  }

  source_file->text = file_text;
  source_file->name = file_name;
  source_file->line_offsets = line_offsets;
  source_file->labels = labels;
  source_file->diagnostics = errors;
  source_file->tokens = tokens;
  source_file->parsed_items = parsed_items;
  source_file->buffer = buffer;

  return source_file;

fail:
  free(source_file);
  vector_free(line_offsets);
  vector_free(labels);
  vector_free(errors);
  vector_free(tokens);
  vector_free(parsed_items);

  return NULL;
}

static void source_file_free(struct SourceFile *source_file) {
  vector_free(source_file->line_offsets);
  vector_free(source_file->diagnostics);
  vector_free(source_file->labels);

  // Free token strings.
  for (int i = 0; i < vector_len(source_file->tokens); i++) {
    struct Token *token = vector_get_elem(source_file->tokens, i);
    free(token->text);
  }

  vector_free(source_file->tokens);
  vector_free(source_file->parsed_items);
  vector_free(source_file->buffer);

  free(source_file->text);
  free(source_file);
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

static void lexer_print_tokens(struct SourceFile *source_file) {
  for (int i = 0; i < vector_len(source_file->tokens); i++) {
    struct Token *token = vector_get_elem(source_file->tokens, i);
    switch (token->kind) {
      case TK_IDENT:
        printf("IDENT: %s\n", token->text);
        break;
      case TK_NONE:
        printf("NONE\n");
        break;
      case TK_INVALID:
        printf("INVALID: %s\n", token->text);
        break;
      case TK_PUNCT:
        printf("PUNCT %s\n", token->text);
        break;
      case TK_NEWLINE:
        printf("NEWLINE\n");
        break;
      case TK_NUM:
        printf("NUM: %s, %d\n", token->text, (int)token->value);
        break;
    }
  }
}

static void parser_print_items(struct SourceFile *source_file) {
  const char *enc_str[] = {
    "ENC_NONE", "ENC_FIXED", "ENC_REG_REG", "ENC_REG_ID",
    "ENC_REG_ARITH", "ENC_REG_PAIR_SP", "ENC_REG_PAIR_BD",
    "ENC_REG_PAIR_PSW", "ENC_REG_PAIR_SP_IMM16", "ENC_REG_IMM8",
    "ENC_IMM8", "ENC_ADDR16", "ENC_RST"
  };

  for (int i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    switch (p->type) {
      case PARSED_NONE:
        break;
      case PARSED_LABEL:
        printf("LABEL:\n");
        printf("      %s\n", p->label.text);
        break;
      case PARSED_INSTR:
        printf("INSTR:\n");
        printf("      enc : %s\n", enc_str[p->instr.enc_t]);
        printf("      op  : %x\n", p->instr.opcode);

        printf("      num : ");
        switch (p->instr.number.type) {
          case OP_UNRESOLVED:
            printf("%s (unresolved)\n", p->instr.number.unresolved);
            break;
          case OP_VALUE:
            printf("%x\n", p->instr.number.value);
            break;
        }

        printf("      r1  : %d\n", p->instr.reg1);
        printf("      r2  : %d\n", p->instr.reg2);
        break;
    }
  }
}

int main(int argc, char **argv) {
  if (argc != 3) {
    printf("expected \"%s <file.asm-to-read> <file.bin-to-write>\"\n", argv[0]);
    return 1;
  }

  const char *file_read_name = argv[1];
  const char *file_write_name = argv[2];

  char *file_text = get_file_text(file_read_name);
  if (!file_text) {
    printf("Reading \'%s\' into a buffer failed", file_read_name);
    return 1;
  }

  struct SourceFile *source_file = source_file_new(file_read_name, file_text);

  lexer_tokenize(source_file);
  parser_parse(source_file);
  labels_build_table(source_file);
  labels_resolve_all(source_file);
  diagnostics_print(source_file);

  if (!vector_len(source_file->diagnostics)) {
    assemble(source_file);
    FILE *fpw = fopen(file_write_name, "wb");
    if (fpw) {
      uint8_t *buffer_data = vector_get_data(source_file->buffer);
      size_t buffer_len = vector_len(source_file->buffer);
      size_t written = fwrite(buffer_data, sizeof(uint8_t), buffer_len, fpw);
      if (written != buffer_len) fprintf(stderr, "Writing buffer to \'%s\' failed\n", file_write_name);
      else printf("Written to %s\n", file_write_name);
      fclose(fpw);
    }
  }

  source_file_free(source_file);
  return 0;
}
