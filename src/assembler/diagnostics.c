#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include "util/slice.h"
#include "util/vector.h"

#include "lexer.h"
#include "diagnostics.h"

#define RED   "\033[31m"
#define RESET "\033[0m"

static bool diagnostic_insert(struct SourceFile *source_file, const struct Diagnostic *diagnostic) {
  size_t d_sz = vector_len(source_file->diagnostics);
  size_t i = 0;

  while (i < d_sz) {
    struct Diagnostic *d = vector_get_elem(source_file->diagnostics, i);
    if (d->range.lineno == diagnostic->range.lineno) {
      if (d->range.start_offset > diagnostic->range.start_offset) {
        break;
      }
    } else if (d->range.lineno > diagnostic->range.lineno) {
      break;
    }
    i++;
  }

  return vector_insert(source_file->diagnostics, i, diagnostic);
}

bool diagnostics_add(struct SourceFile *source_file, struct SourceRange range, const char *fmt, ...) {
  if (!source_file || !fmt) return false;

  struct Diagnostic diagnostic = {
    .range = range,
    .fmt = fmt,
    .arg_count = 0,
  };

  va_list ap;
  va_start(ap, fmt);

  for (const char *p = fmt; *p; p++) {
    if (*p == '%' && *(p + 1)) {
      p++;
      if (*p == 's') {
        if (diagnostic.arg_count < DIAG_MAX_ARGS) {
          diagnostic.args[diagnostic.arg_count++] = (struct DiagArg){
            .kind = ARG_SLICE,
            .slice = va_arg(ap, struct Slice),
          };
        }
      } else if (*p == 'n') {
        if (diagnostic.arg_count < DIAG_MAX_ARGS) {
          diagnostic.args[diagnostic.arg_count++] = (struct DiagArg){
            .kind = ARG_NUM,
            .num = va_arg(ap, uint64_t),
          };
        }
      } else if (*p == '%') {
        // escaped %%
      }
    }
  }

  va_end(ap);

  return diagnostic_insert(source_file, &diagnostic);
}

static void fprint_diag(FILE *out, const struct Diagnostic *d) {
  size_t arg_idx = 0;

  for (const char *p = d->fmt; *p; ) {
    if (*p != '%') {
      fputc(*p++, out);
      continue;
    }

    p++; // skip '%'
    if (*p == 's') {
      if (arg_idx < d->arg_count && d->args[arg_idx].kind == ARG_SLICE) {
        slice_print(out, d->args[arg_idx].slice);
        arg_idx++;
      }
      p++;
    } else if (*p == 'n') {
      if (arg_idx < d->arg_count && d->args[arg_idx].kind == ARG_NUM) {
        fprintf(out, "%llu", (unsigned long long)d->args[arg_idx].num);
        arg_idx++;
      }
      p++;
    } else if (*p == '%') {
      fputc('%', out);
      p++;
    } else {
      fputc('%', out);
      if (*p) fputc(*p++, out);
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
