#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "util/slice.h"

bool slice_eq(struct Slice a, struct Slice b) {
  if (a.len != b.len) return false;
  if (a.len == 0) return true;
  if (!a.text || !b.text) return false;
  return memcmp(a.text, b.text, a.len) == 0;
}

bool slice_eq_nocase(struct Slice s, const char *lit) {
  size_t n = strlen(lit);
  if (s.len != n) return false;
  for (size_t i = 0; i < n; i++)
    if (toupper((unsigned char)s.text[i]) != toupper((unsigned char)lit[i]))
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
