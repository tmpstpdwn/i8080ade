#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>

#include "assembler/lexer.h"
#include "util/vector.h"
#include "opcode.h"
#include "lexer.h"
#include "diagnostics.h"
#include "parser.h"
#include "assembler.h"

enum InstrGrammarElem {
  IGE_OPCODE,
  IGE_COMMA,
  IGE_REG,
  IGE_D8,
  IGE_D16,
  IGE_A16,
  IGE_END
};

struct InstrGrammar {
  enum InstrGrammarElem arr[5];
};

static const struct InstrGrammar grammar_fixed   = {{IGE_OPCODE, IGE_END}};
static const struct InstrGrammar grammar_reg     = {{IGE_OPCODE, IGE_REG, IGE_END}};
static const struct InstrGrammar grammar_reg_reg = {{IGE_OPCODE, IGE_REG, IGE_COMMA, IGE_REG, IGE_END}};
static const struct InstrGrammar grammar_reg_d8  = {{IGE_OPCODE, IGE_REG, IGE_COMMA, IGE_D8, IGE_END}};
static const struct InstrGrammar grammar_reg_d16 = {{IGE_OPCODE, IGE_REG, IGE_COMMA, IGE_D16, IGE_END}};
static const struct InstrGrammar grammar_d8      = {{IGE_OPCODE, IGE_D8, IGE_END}};
static const struct InstrGrammar grammar_a16     = {{IGE_OPCODE, IGE_A16, IGE_END}};

static const struct InstrGrammar *grammar_for(enum EncType enc_t) {
  switch (enc_t) {
    case ENC_NONE:
      return NULL;
    case ENC_FIXED:
      return &grammar_fixed;
    case ENC_REG_REG:
      return &grammar_reg_reg;
    case ENC_REG_ID:
    case ENC_REG_ARITH:
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      return &grammar_reg;
    case ENC_REG_PAIR_SP_IMM16:
      return &grammar_reg_d16;
    case ENC_REG_IMM8:
      return &grammar_reg_d8;
    case ENC_IMM8:
    case ENC_RST:
      return &grammar_d8;
    case ENC_ADDR16:
      return &grammar_a16;
  }
  return NULL;
}

static bool is_mnemonic(const char *s) {
  if (!s) return false;
  struct OpcodeRec oprec;
  return opcode_rec_get(s, &oprec);
}

static bool is_reg(const char *s) {
  if (!s) return false;
  return (opcode_reg_code(s, ENC_REG_REG) != -1 ||
          opcode_reg_code(s, ENC_REG_PAIR_PSW) != -1 ||
          opcode_reg_code(s, ENC_REG_PAIR_SP) != -1 );
}

static int digit_value(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  return -1;
}

static bool parse_number(const char *str, uint64_t *num) {
  if (!str || !*str || !num) return false;

  int base = 10;
  const char *p = str;

  if (p[0] == '0') {
    switch (tolower((unsigned char)p[1])) {
      case 'x': base = 16; break;
      case 'b': base = 2;  break;
      case 'o': base = 8;  break;
    }
    if (base != 10) p += 2;
  }

  if (!*p) return false;

  uint64_t v = 0;
  for (; *p; p++) {
    int d = digit_value((unsigned char)*p);
    if (d < 0 || d >= base) return false;
    if (v > (UINT64_MAX - (uint64_t)d) / (uint64_t)base) return false;
    v = v * (uint64_t)base + (uint64_t)d;
  }

  *num = v;
  return true;
}

static bool add_parsed_item(struct SourceFile *source_file, struct ParsedItem parsed_item) {
  if (!source_file) return false;
  return vector_append(source_file->parsed_items, &parsed_item);
}

static void consume_line(struct SourceFile *source_file) {
  size_t i = 1;
  struct Token *token;

  while ((token = lexer_peek_token(source_file, i)) && token->kind != TK_NEWLINE)
    i++;

  lexer_consume_token(source_file, i);
}

// Returns  1: Successfully parsed a label.
// Returns  0: Unsuccessfull at parsing a label, but there are tokens left in the line.
// Returns -1: Unsuccessfull at parsing a label, no more tokens left in the line.
static int parse_label(struct SourceFile *source_file) {
  if (!source_file) return -1;

  struct Token *first_token = lexer_peek_token(source_file, 1);
  struct Token *second_token = lexer_peek_token(source_file, 2);

  if (!first_token || !second_token) return -1;

  if (second_token->kind == TK_PUNCT && strcmp(second_token->text, ":") == 0) {
    if (first_token->kind == TK_IDENT) {
      if (is_mnemonic(first_token->text)) {
        diagnostics_add(source_file, (struct Diagnostic){"Instruction mnemonic can't be used as a label", first_token->range});
      }
      else if (is_reg(first_token->text)) {
        diagnostics_add(source_file, (struct Diagnostic){"Register name can't be used as a label", first_token->range});
      }
      else {
        struct ParsedLabel label = {first_token->text, first_token->range};
        add_parsed_item(source_file, (struct ParsedItem){.type = PARSED_LABEL, .label = label});
      }
    } else {
      diagnostics_add(source_file, (struct Diagnostic){"Expected a valid label identifier", first_token->range});
    }
    lexer_consume_token(source_file, 2);

    if (lexer_peek_token(source_file, 1) && lexer_peek_token(source_file, 1)->kind == TK_NEWLINE) {
      lexer_consume_token(source_file, 1);    
      return -1;
    }
  } else {
    return 0;
  }
  
  return 1;
}

static void parse_instr(struct SourceFile *source_file) {
  if (!source_file) return;

  struct OpcodeRec oprec;
  const enum InstrGrammarElem *grammar_arr = NULL;
  size_t grammar_arr_i = 0;
  struct ParsedInstr parsed_instr = {0};

  while (true) {
    struct Token *token = lexer_peek_token(source_file, 1);
    if (!token) return;

    if (grammar_arr == NULL) {
      if (token->kind == TK_IDENT && opcode_rec_get(token->text, &oprec)) {
        const struct InstrGrammar *grammar = grammar_for(oprec.enc_t);
        if (!grammar) goto fail;
        grammar_arr = grammar->arr;
      } else {
        diagnostics_add(source_file, (struct Diagnostic){"Expected a valid opcode mnemonic", token->range});
        goto fail;
      }
    }

    enum InstrGrammarElem curr_ige = grammar_arr[grammar_arr_i];

    if (token->kind == TK_NEWLINE && curr_ige != IGE_END) {
      struct LineOffset *lo = vector_get_elem(source_file->line_offsets, token->range.lineno);
      struct SourceRange range = {0};
      if (lo) range = (struct SourceRange){token->range.lineno, lo->offset, lo->offset + lo->len};
      diagnostics_add(source_file, (struct Diagnostic){"Incomplete instruction, expected more operands", range});
      goto fail;
    }

    switch (curr_ige) {
      case IGE_OPCODE:
        parsed_instr.enc_t = oprec.enc_t;
        parsed_instr.opcode = oprec.opcode;
        break;

      case IGE_COMMA:
        if (token->kind != TK_PUNCT || token->text[0] != ',') {
          diagnostics_add(source_file, (struct Diagnostic){"Expected \',\' between operands", token->range});
          goto fail;
        }
        break;

      case IGE_D8:
      case IGE_D16:
      case IGE_A16:
        if (token->kind == TK_NUM) {
          if (parse_number(token->text, &parsed_instr.number.value)) {
            parsed_instr.number.type = OP_VALUE;
          } else {
            diagnostics_add(source_file, (struct Diagnostic){"Received a malformed number literal", token->range});
            goto fail;
          }
        } else {
          if (curr_ige == IGE_A16) {
            if (token->kind == TK_IDENT) {
              parsed_instr.number.type = OP_LABEL;
              parsed_instr.number.label = token->text;
            } else {
              diagnostics_add(source_file, (struct Diagnostic){"Expected a valid number literal or a label", token->range});
              goto fail;
            }
          } else {
            diagnostics_add(source_file, (struct Diagnostic){"Expected a valid number literal", token->range});
            goto fail;
          }
        }

        parsed_instr.number.range = token->range;
        break;

      case IGE_REG: {
        int register_c = opcode_reg_code(token->text, oprec.enc_t);
        if (register_c == -1) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a register", token->range});
          goto fail;
        }

        if (grammar_arr_i == 1) {
          parsed_instr.reg1.type = OP_VALUE;
          parsed_instr.reg1.range = token->range;
          parsed_instr.reg1.value = register_c;
        } else {
          parsed_instr.reg2.type = OP_VALUE;
          parsed_instr.reg2.range = token->range;
          parsed_instr.reg2.value = register_c;          
        }
        break;
      }

      case IGE_END:
        if (token->kind != TK_NEWLINE) {
          diagnostics_add(source_file, (struct Diagnostic){"Unexpected trailing tokens", token->range});
          goto fail;
        }
        add_parsed_item(source_file, (struct ParsedItem){.type = PARSED_INSTR, .instr = parsed_instr});
        break;
    }

    lexer_consume_token(source_file, 1);
    grammar_arr_i++;

    if (token->kind == TK_NEWLINE) break;
  }

  // Should only be reached once the whole line is done with.
  return; 

fail:
  // In case parsing a line fails, consume all tokens left in the line.
  consume_line(source_file);
  return;
}

void parser_parse(struct SourceFile *source_file) {
  int r;
  while (lexer_peek_token(source_file, 1)) {
    while((r = parse_label(source_file)) > 0);
    if (r == -1) continue;
    parse_instr(source_file);
  }
}

void parser_validate(struct SourceFile *source_file) {
  for (size_t i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    if (p->type != PARSED_INSTR) continue; 

    switch (p->instr.enc_t) {
      case ENC_REG_REG:
        if (p->instr.reg1.value == 6 && p->instr.reg2.value == 6) {
          struct SourceRange range = {p->instr.reg1.range.lineno, p->instr.reg1.range.start_offset, p->instr.reg2.range.end_offset};
          diagnostics_add(source_file, (struct Diagnostic){"Invalid Operands", range});
        }
        break;
      case ENC_ADDR16:
        if (p->instr.number.type == OP_VALUE && p->instr.number.value > 0xFFFF)
          diagnostics_add(source_file, (struct Diagnostic){"Expected an address within the range 0x0000 - 0xFFFF (16-bit)", p->instr.number.range});
        break;
      case ENC_REG_PAIR_SP_IMM16:
        if (p->instr.number.value > 0xFFFF)
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", p->instr.number.range});
        break;
      case ENC_IMM8:
      case ENC_REG_IMM8:
        if (p->instr.number.value > 0xFF)
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x00 - 0xFF (8-bit)", p->instr.number.range});
        break;
      case ENC_RST:
        if (p->instr.number.value >= 8)
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0 - 0x7 (3-bit)", p->instr.number.range});
        break;
      default: break;
    }

  }
}

void parser_print_items(struct SourceFile *source_file) {
  const char *enc_str[] = {
    "ENC_NONE", "ENC_FIXED", "ENC_REG_REG", "ENC_REG_ID",
    "ENC_REG_ARITH", "ENC_REG_PAIR_SP", "ENC_REG_PAIR_BD",
    "ENC_REG_PAIR_PSW", "ENC_REG_PAIR_SP_IMM16", "ENC_REG_IMM8",
    "ENC_IMM8", "ENC_ADDR16", "ENC_RST"
  };

  for (size_t i = 0; i < vector_len(source_file->parsed_items); i++) {
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
        printf("      op  : 0x%x\n", p->instr.opcode);

        printf("      num : ");
        switch (p->instr.number.type) {
          case OP_LABEL:
            printf("%s (unresolved)\n", p->instr.number.label);
            break;
          case OP_VALUE:
            printf("0x%x\n", (int)p->instr.number.value);
            break;
        }

        printf("      r1  : %d\n", (int)p->instr.reg1.value);
        printf("      r2  : %d\n", (int)p->instr.reg2.value);
        break;
    }
  }
}
