#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>

#include "lexer.h"

struct SourceFile;

struct Diagnostic {
  char *text;
  struct SourceRange range;
};

bool diagnostics_add(struct SourceFile *source_file, struct Diagnostic diagnostic);
void diagnostics_print(struct SourceFile *source_file);

#endif
