#ifndef LABELS_H
#define LABELS_H

#include "util/slice.h"
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct SourceFile;

struct Label {
  struct Slice text;
  uint16_t addr;
};

void labels_build_table(struct SourceFile *source_file);
void labels_resolve_all(struct SourceFile *source_file);

#endif
