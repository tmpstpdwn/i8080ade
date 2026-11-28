#include <stdbool.h>
#include <stdlib.h>

#include "source.h"
#include "i8080.h"
#include "lexer.h"
#include "diagnostics.h"
#include "parser.h"
#include "labels.h"
#include "assembler.h"

#include "util/vector.h"
#include "util/file.h"

static bool assemble(struct SourceFile *source_file, struct Vector *buffer) {
  if (!source_file || !buffer) return false;

  for (size_t i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);

    switch (p->type) {
      case PARSED_INSTR: {
        uint8_t size = i8080_instr_size(p->instr.enc_t);
        struct I8080Instr instr = {
          p->instr.opcode, p->instr.enc_t, p->instr.reg1.value,
          p->instr.reg2.value, p->instr.number.value, size
        };
        uint32_t assembled_instr = i8080_encode_instr(instr);
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

static struct Vector *assemble_source(const char *file_text, const char *file_name, struct Vector *buffer) {
  if (!file_text || !file_name || !buffer) return NULL;

  struct Vector *result = NULL;

  struct SourceFile *source_file = source_file_new(file_name, file_text);
  if (!source_file) return NULL;

  lexer_tokenize(source_file);
  parser_parse(source_file);
  labels_build_table(source_file);
  labels_resolve_all(source_file);
  parser_validate(source_file);

  if (vector_len(source_file->diagnostics)) {
    diagnostics_print(source_file);
  } else if (assemble(source_file, buffer)) {
    result = buffer;
  }

  source_file_free(source_file);
  return result;
}

bool assemble_file(const char *in, const char *out) {
  char *text = file_read(in);
  if (!text) {
    fprintf(stderr, "Error: cannot read '%s'\n", in);
    return false;
  }

  struct Vector *buf = vector_new(sizeof(uint8_t));
  if (!buf) {
    fprintf(stderr, "Error: out of memory\n");
    free(text);
    return false;
  }

  if (assemble_source(text, in, buf)) {
    if (file_write(out, vector_get_data(buf), vector_len(buf))) {
      printf("Written to %s\n", out);
    } else {
      fprintf(stderr, "Error: cannot write '%s'\n", out);
    }
  }

  free(text);
  vector_free(buf);
  return true;
}
