#ifndef SOURCE_H
#define SOURCE_H

#include <stddef.h>

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

struct SourceFile *source_file_new(const char *file_name, const char *file_text);
void source_file_free(struct SourceFile *source_file);

#endif
