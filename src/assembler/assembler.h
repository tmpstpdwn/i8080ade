#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "util/vector.h"

struct LineOffset {
  size_t offset;
  size_t len;
};

struct SourceFile {
  const char *text;
  const char *name;
  struct Vector *line_offsets;
  struct Vector *labels;
  struct Vector *tokens;
  struct Vector *diagnostics;
  struct Vector *parsed_items;
  size_t token_cursor;
};

struct Vector *assemble_source(const char *file_text, const char *file_name, struct Vector *buffer);

#endif
