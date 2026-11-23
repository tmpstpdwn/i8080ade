#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "util/vector.h"
#include "i8080.h"
#include "lexer.h"
#include "diagnostics.h"
#include "parser.h"
#include "labels.h"
#include "assembler.h"

static bool labels_add(struct SourceFile *source_file, struct Label label) {
  if (!source_file) return false;
  return vector_append(source_file->labels, &label);
}

static bool labels_resolve(struct SourceFile *source_file, struct Slice name,
                           uint16_t *resolved_val) {
  if (!source_file || !resolved_val || !name.text) return false;

  size_t label_table_len = vector_len(source_file->labels);

  for (size_t i = 0; i < label_table_len; i++) {
    const struct Label *label = vector_get_elem(source_file->labels, i);
    if (slice_eq(label->text, name)) {
      *resolved_val = label->addr;
      return true;
    }
  }

  return false;
}

void labels_build_table(struct SourceFile *source_file) {
  uint16_t address_counter = 0;

  size_t parsed_items_len = vector_len(source_file->parsed_items);
  for (size_t i = 0; i < parsed_items_len; i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    uint16_t dummy;
    switch (p->type) {
      case PARSED_NONE:
        break;
      case PARSED_LABEL:
        if (labels_resolve(source_file, p->label.text, &dummy)) {
          diagnostics_add(source_file, (struct Diagnostic){"Duplicate label", p->label.range});
        } else {
          labels_add(source_file, (struct Label){p->label.text, address_counter});
        }
        break;
      case PARSED_INSTR:
        address_counter += i8080_instr_size(p->instr.enc_t);
        break;
    }
  }
}

void labels_resolve_all(struct SourceFile *source_file) {
  size_t parsed_items_len = vector_len(source_file->parsed_items);
  for (size_t i = 0; i < parsed_items_len; i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    if (p->type == PARSED_INSTR) {
      if (p->instr.number.type == OP_LABEL) {
        uint16_t resolved_val = 0;
        if (labels_resolve(source_file, p->instr.number.label, &resolved_val)) {
          p->instr.number.type = OP_VALUE;
          p->instr.number.value = resolved_val;
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Undefined label", p->instr.number.range});
        }
      }
    }
  }
}
