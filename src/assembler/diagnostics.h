#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lexer.h"
#include "util/slice.h"
#include "source.h"

enum DiagArgKind {
  ARG_SLICE,
  ARG_NUM,
};

struct DiagArg {
  enum DiagArgKind kind;
  union {
    struct Slice slice;
    uint64_t num;
  };
};

#define DIAG_MAX_ARGS 4

struct Diagnostic {
  struct SourceRange range;
  const char *fmt;
  uint8_t arg_count;
  struct DiagArg args[DIAG_MAX_ARGS];
};

bool diagnostics_add(struct SourceFile *source_file, struct SourceRange range, const char *fmt, ...);
void diagnostics_print(struct SourceFile *source_file);

#endif
