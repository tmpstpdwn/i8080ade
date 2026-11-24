#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "util/slice.h"
#include "util/vector.h"

#include "lexer.h"
#include "diagnostics.h"

#define RED   "\033[31m"
#define RESET "\033[0m"

static const char *const diag_fmt[DIAG_COUNT] = {
#define X(id, fmt) [DIAG_##id] = fmt,
  DIAG_LIST(X)
#undef X
};

bool diagnostics_add(struct SourceFile *source_file, enum DiagId id, struct SourceRange range, struct DiagArgs args) {
  if (!source_file) return false;

  struct Diagnostic diagnostic = {id, range, args};

  size_t d_sz = vector_len(source_file->diagnostics);
  size_t i = 0;

  while (i < d_sz) {
    struct Diagnostic *d = vector_get_elem(source_file->diagnostics, i);
    if (d->range.lineno == diagnostic.range.lineno) {
      if (d->range.start_offset > diagnostic.range.start_offset) {
        break;
      }
    } else if (d->range.lineno > diagnostic.range.lineno) {
      break;
    }
    i++;
  }

  return vector_insert(source_file->diagnostics, i, &diagnostic);
}

static bool is_placeholder(const char **p, const char *placeholder) {
  size_t n = strlen(placeholder);
  if (strncmp(*p, placeholder, n) != 0) return false;
  *p += n;
  return true;
}

static void fprint_diag(FILE *out, const struct Diagnostic *d) {
  const struct DiagArgs *a = &d->args;

  for (const char *p = diag_fmt[d->id]; *p; ) {
    if (*p != '{') { fputc(*p++, out); continue; }
    if (is_placeholder(&p, "{name}")) {
      slice_print(out, a->name);
    } else if (is_placeholder(&p, "{num}")) {
      fprintf(out, "%llu", (unsigned long long)a->num);
    } else if (is_placeholder(&p, "{bits}")) {
      fprintf(out, "%llu", (unsigned long long)a->bits);
    } else {
      fputc(*p++, out);
    }
  }

}

void diagnostics_print(struct SourceFile *source_file) {
  if (!source_file) return;

  for (size_t i = 0; i < vector_len(source_file->diagnostics); i++) {
    struct Diagnostic *d = vector_get_elem(source_file->diagnostics, i);

    size_t line = d->range.lineno;

    struct LineOffset *line_offset = vector_get_elem(source_file->line_offsets, line);

    size_t start = d->range.start_offset;
    size_t end = d->range.end_offset;

    size_t offset = line_offset->offset;
    size_t len = line_offset->len;

    if (start < offset || start > offset + len)
      continue;

    size_t column = start - offset;

    if (end > offset + len)
      end = offset + len;

    size_t highlight_len = end > start ? end - start : 1;

    fprintf(stderr, "%s:%zu:%zu: %serror:%s ",
            source_file->name, line + 1, column + 1,
            RED, RESET);

    fprint_diag(stderr, d);
    fputc('\n', stderr);

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
