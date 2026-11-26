#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

#include "util/slice.h"

struct Slice slice_from(const char *s, size_t from, size_t to) {
  if (!s) return (struct Slice){0};

  size_t len = strlen(s);

  if (from >= to || to > len)
    return (struct Slice){0};

  return (struct Slice) {
    .text = s + from,
    .len = to - from
  };
}

bool slice_eq(struct Slice a, struct Slice b, bool nocase) {
  if (a.len != b.len) return false;
  if (a.len == 0) return true;
  if (!a.text || !b.text) return false;

  if (!nocase)
    return memcmp(a.text, b.text, a.len) == 0;

  for (size_t i = 0; i < a.len; i++)
    if (toupper((unsigned char)a.text[i]) !=
        toupper((unsigned char)b.text[i]))
      return false;

  return true;
}

bool slice_cpy(char *dst, size_t cap, struct Slice src) {
  if (!dst || cap == 0) return false;
  dst[0] = '\0';
  if (!src.text && src.len > 0) return false;

  size_t len = src.len;
  bool fits = len < cap;
  if (!fits) len = cap - 1;

  if (len > 0) memcpy(dst, src.text, len);
  dst[len] = '\0';
  return fits;
}

void slice_print(FILE *out, struct Slice s) {
  for (size_t i = 0; i < s.len; i++) {
    unsigned char c = (unsigned char)s.text[i];
    fputc(c, out);
  }
}
