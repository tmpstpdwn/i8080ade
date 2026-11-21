#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "util/vector.h"
#include "opcode.h"
#include "lexer.h"
#include "diagnostics.h"
#include "parser.h"
#include "labels.h"
#include "assembler.h"

static uint32_t assemble_instr(struct ParsedInstr *parsed_instr) {
  uint32_t asmbld_instr = 0;

  enum EncType enc_t = parsed_instr->enc_t;
  uint8_t opcode = parsed_instr->opcode;

  uint8_t register1_c = parsed_instr->reg1.value;
  uint8_t register2_c = parsed_instr->reg2.value;

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

static bool assemble(struct SourceFile *source_file, struct Vector *buffer) {
  if (!source_file || !buffer) return false;

  for (size_t i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);

    switch (p->type) {
      case PARSED_INSTR: {
        uint8_t size = opcode_instr_size(p->instr.enc_t);
        uint32_t assembled_instr = assemble_instr(&p->instr);
        while (size) {
          uint8_t b = assembled_instr & 0xFF; 
          if (!vector_append(buffer, &b)) return false;
          assembled_instr = assembled_instr >> 8;
          size--;
        }
        break;
      }
      default:
        break;
    }

  }
  return vector_len(buffer) > 0;
}

static struct SourceFile *source_file_new(const char *file_name, const char *file_text) {
  if (!file_text || !file_name) return NULL;

  struct SourceFile *source_file = malloc(sizeof(struct SourceFile));

  struct Vector *line_offsets = vector_new(sizeof(struct LineOffset));
  struct Vector *labels = vector_new(sizeof(struct Label));
  struct Vector *errors = vector_new(sizeof(struct Diagnostic));
  struct Vector *tokens = vector_new(sizeof(struct Token));
  struct Vector *parsed_items = vector_new(sizeof(struct ParsedItem));

  if (!source_file || !line_offsets || !errors ||
      !tokens || !parsed_items) goto fail;

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
  source_file->token_cursor = 0;

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
  for (size_t i = 0; i < vector_len(source_file->tokens); i++) {
    struct Token *token = vector_get_elem(source_file->tokens, i);
    free(token->text);
  }

  vector_free(source_file->tokens);
  vector_free(source_file->parsed_items);

  free(source_file);
}

struct Vector *assemble_source(char *s, const char *file_name, struct Vector *buffer) {
  if (!s || !file_name || !buffer) return NULL;

  struct SourceFile *source_file = source_file_new(file_name, s);
  if (!source_file) return NULL;

  lexer_tokenize(source_file);
  parser_parse(source_file);
  parser_validate(source_file);
  labels_build_table(source_file);
  labels_resolve_all(source_file);
  diagnostics_print(source_file);

  struct Vector *result = NULL;
  if (vector_len(source_file->diagnostics) == 0) {
    if (assemble(source_file, buffer)) {
      result = buffer;
    }
  }

  source_file_free(source_file);
  return result;
}
