#include <stdio.h>
#include <ctype.h>

#include "util/vector.h"
#include "i8080.h"
#include "lexer.h"
#include "diagnostics.h"
#include "parser.h"

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

static bool is_mnemonic(struct Slice s) {
  struct OpcodeRec dummy;
  return i8080_opcode_rec(s, &dummy);
}

static bool is_reg(struct Slice s) {
  return i8080_reg_code(s, ENC_REG_REG)     != -1 ||
         i8080_reg_code(s, ENC_REG_PAIR_PSW) != -1 ||
         i8080_reg_code(s, ENC_REG_PAIR_SP)  != -1;
}

static int digit_value(int c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'z') return c - 'a' + 10;
  if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
  return -1;
}

static bool parse_number(struct Slice s, uint64_t *num) {
  if (!num || !s.text || s.len == 0) return false;

  int base = 10;
  size_t i = 0;

  if (s.len >= 2 && s.text[0] == '0') {
    switch (tolower((unsigned char)s.text[1])) {
      case 'x': base = 16; break;
      case 'b': base = 2;  break;
      case 'o': base = 8;  break;
      default: break;
    }
    if (base != 10) i = 2;
  }

  if (i >= s.len) return false;

  uint64_t v = 0;
  for (; i < s.len; i++) {
    int d = digit_value((unsigned char)s.text[i]);
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

  // No more tokens.
  if (!first_token || !second_token) return -1;

  // Skip parsing the whole line in case it contains an invalid token,
  // which would have already been reported of at the lexer.
  if (first_token->kind == TK_INVALID || second_token->kind == TK_INVALID) {
    consume_line(source_file);
    return -1;    
  }

  char punct = source_file->text[second_token->range.start_offset];
  if (second_token->kind == TK_PUNCT && punct == ':') {
    if (first_token->kind == TK_IDENT) {
      if (is_mnemonic(first_token->slice)) {
        diagnostics_add(source_file, first_token->range, "'%s' is an instruction mnemonic and can't be used as a label", first_token->slice);
      }
      else if (is_reg(first_token->slice)) {
        diagnostics_add(source_file, first_token->range, "'%s' is a register name and can't be used as a label", first_token->slice);
      }
      else {
        struct ParsedLabel label = {first_token->slice, first_token->range};
        add_parsed_item(source_file, (struct ParsedItem){.type = PARSED_LABEL, .label = label});
      }
    } else {
      diagnostics_add(source_file, first_token->range, "'%s' is not a valid label name: labels must start with a letter or '_'", first_token->slice);
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
      if (token->kind == TK_IDENT && i8080_opcode_rec(token->slice, &oprec)) {
        const struct InstrGrammar *grammar = grammar_for(oprec.enc_t);
        if (!grammar) goto fail;
        grammar_arr = grammar->arr;
      } else {
        diagnostics_add(source_file, token->range, "expected an instruction mnemonic, found '%s'", token->slice);
        goto fail;
      }
    }

    enum InstrGrammarElem curr_ige = grammar_arr[grammar_arr_i];

    if (token->kind == TK_NEWLINE && curr_ige != IGE_END) {
      struct LineOffset *lo = vector_get_elem(source_file->line_offsets, token->range.lineno);
      struct SourceRange range = {0};
      if (lo) range = (struct SourceRange){token->range.lineno, lo->offset, lo->offset + lo->len};
      diagnostics_add(source_file, range, "incomplete instruction, expected more operands");
      goto fail;
    }

    switch (curr_ige) {
      case IGE_OPCODE:
        parsed_instr.enc_t = oprec.enc_t;
        parsed_instr.opcode = oprec.opcode;
        parsed_instr.range = token->range;
        break;

      case IGE_COMMA:
        if (token->kind != TK_PUNCT || token->slice.text[0] != ',') {
          diagnostics_add(source_file, token->range, "expected ',' between operands, found '%s'", token->slice);
          goto fail;
        }
        break;

      case IGE_D8:
      case IGE_D16:
      case IGE_A16:
        if (token->kind == TK_NUM) {
          if (parse_number(token->slice, &parsed_instr.number.value)) {
            parsed_instr.number.type = OP_VALUE;
          } else {
            diagnostics_add(source_file, token->range, "malformed number literal '%s'", token->slice);
            goto fail;
          }
        } else {
          if (curr_ige == IGE_A16) {
            if (token->kind == TK_IDENT) {
              parsed_instr.number.type = OP_LABEL;
              parsed_instr.number.label = token->slice;
            } else {
              diagnostics_add(source_file, token->range, "expected a number or a label, found '%s'", token->slice);
              goto fail;
            }
          } else {
            diagnostics_add(source_file, token->range, "expected a number, found '%s'", token->slice);
            goto fail;
          }
        }

        parsed_instr.number.range = token->range;
        break;

      case IGE_REG: {
        int register_c = i8080_reg_code(token->slice, oprec.enc_t);
        if (register_c == -1) {
          diagnostics_add(source_file, token->range, "expected a register, found '%s'", token->slice);
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
          diagnostics_add(source_file, token->range, "unexpected '%s' after the end of the instruction", token->slice);
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
          diagnostics_add(source_file, range, "'MOV M, M' is not a valid instruction (that encoding is HLT)");
        }
        break;
      case ENC_ADDR16:
        if (p->instr.number.type == OP_VALUE && p->instr.number.value > 0xFFFF)
          diagnostics_add(source_file, p->instr.number.range, "%n does not fit in %n bits", (uint64_t)p->instr.number.value, (uint64_t)16);
        break;
      case ENC_REG_PAIR_SP_IMM16:
        if (p->instr.number.value > 0xFFFF)
          diagnostics_add(source_file, p->instr.number.range, "%n does not fit in %n bits", (uint64_t)p->instr.number.value, (uint64_t)16);
        break;
      case ENC_IMM8:
      case ENC_REG_IMM8:
        if (p->instr.number.value > 0xFF)
          diagnostics_add(source_file, p->instr.number.range, "%n does not fit in %n bits", (uint64_t)p->instr.number.value, (uint64_t)8);
        break;
      case ENC_RST:
        if (p->instr.number.value >= 8)
          diagnostics_add(source_file, p->instr.number.range, "%n does not fit in %n bits", (uint64_t)p->instr.number.value, (uint64_t)3);
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
        printf("      %.*s\n", (int)p->label.text.len, p->label.text.text);
        break;
      case PARSED_INSTR:
        printf("INSTR:\n");
        printf("      enc : %s\n", enc_str[p->instr.enc_t]);
        printf("      op  : 0x%x\n", p->instr.opcode);

        printf("      num : ");
        switch (p->instr.number.type) {
          case OP_LABEL:
            printf("%.*s (unresolved)\n", (int)p->instr.number.label.len, p->instr.number.label.text);
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
