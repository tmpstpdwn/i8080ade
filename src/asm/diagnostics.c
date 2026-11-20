static bool diagnostics_add(struct SourceFile *source_file, struct Diagnostic diagnostic) {
  if (!source_file) return false;
  return vector_append(source_file->diagnostics, &diagnostic);
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
            source_file->name, line + 1, column + 1,
            RED, RESET, diag->text);

    fprintf(stderr, " %4zu | %.*s%s%.*s%s%.*s\n",
            line + 1,
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
