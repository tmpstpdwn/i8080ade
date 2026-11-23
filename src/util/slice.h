#ifndef SLICE_H
#define SLICE_H

#include <stdbool.h>
#include <stddef.h>

struct Slice {
  const char *text;
  size_t len;
};

bool slice_eq(struct Slice a, struct Slice b);
bool slice_eq_nocase(struct Slice s, const char *lit);
bool slice_cpy(char *dst, size_t cap, struct Slice src);

#endif
