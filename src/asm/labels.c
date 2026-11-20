struct Label {
  char *text;
  uint16_t addr;
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
          diagnostics_add(source_file, (struct Diagnostic){"Duplicate label", p->label.range});
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
      if (p->instr.number.type == OP_LABEL) {
        uint16_t resolved_val = 0;
        if (labels_resolve(source_file, p->instr.number.label, &resolved_val)) {
          p->instr.number.type = OP_VALUE;
          p->instr.number.value = resolved_val;
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Undefined Symbol", p->instr.number.range});
        }
      }
    }
  }
}
