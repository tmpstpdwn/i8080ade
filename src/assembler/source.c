#include <stdlib.h>
#include <stddef.h>

#include "source.h"

#include "lexer.h"
#include "parser.h"
#include "diagnostics.h"
#include "labels.h"

#include "util/vector.h"

struct SourceFile *source_file_new(const char *file_name, const char *file_text) {
  if (!file_text || !file_name) return NULL;

  struct SourceFile *source_file = malloc(sizeof(struct SourceFile));

  struct Vector *line_offsets = vector_new(sizeof(struct LineOffset));
  struct Vector *labels = vector_new(sizeof(struct Label));
  struct Vector *diagnostics = vector_new(sizeof(struct Diagnostic));
  struct Vector *tokens = vector_new(sizeof(struct Token));
  struct Vector *parsed_items = vector_new(sizeof(struct ParsedItem));

  if (!source_file || !line_offsets || !labels ||
      !diagnostics || !tokens || !parsed_items) goto fail;

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
  source_file->diagnostics = diagnostics;
  source_file->tokens = tokens;
  source_file->parsed_items = parsed_items;
  source_file->token_cursor = 0;

  return source_file;

fail:
  vector_free(line_offsets);
  vector_free(labels);
  vector_free(diagnostics);
  vector_free(tokens);
  vector_free(parsed_items);
  free(source_file);

  return NULL;
}

void source_file_free(struct SourceFile *source_file) {
  vector_free(source_file->line_offsets);
  vector_free(source_file->diagnostics);
  vector_free(source_file->labels);
  vector_free(source_file->tokens);
  vector_free(source_file->parsed_items);
  free(source_file);
}
