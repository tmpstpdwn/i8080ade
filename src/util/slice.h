#ifndef SLICE_H
#define SLICE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

struct Slice {
  const char *text;
  size_t len;
};

struct Slice slice_from(const char *s, size_t from, size_t to);
bool slice_eq(struct Slice a, struct Slice b, bool nocase);
bool slice_cpy(char *dst, size_t cap, struct Slice src);
void slice_print(FILE *out, struct Slice s);

#endif
