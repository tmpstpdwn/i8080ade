#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

struct Vector {
  void *data;
  size_t cap, len;
  size_t element_sz;
};

struct Vector *vector_new(size_t element_sz) {
  if (!element_sz) return NULL;
  struct Vector *v = malloc(sizeof(struct Vector));
  if (!v) return NULL;
  v->data = NULL;
  v->cap = v->len = 0;
  v->element_sz = element_sz;
  return v;
}

size_t vector_len(struct Vector *v) {
  if (!v) return 0;
  return v->len;
}

bool vector_append(struct Vector *v, const void *value) {
  if (!v || !value) return false;

  if (v->len >= v->cap) {
    size_t new_cap = (v->data == NULL)? 8: v->cap * 2;
    void *tmp = realloc(v->data, v->element_sz * new_cap);
    if (!tmp) return false;
    v->data = tmp;
    v->cap = new_cap;
  }

  memcpy((uint8_t *)v->data + v->len * v->element_sz, value, v->element_sz);

  v->len++;
  return true;
}

bool vector_insert(struct Vector *v, size_t index, const void *value) {
  if (!v || !value || index > v->len) return false;

  if (v->len >= v->cap) {
    size_t new_cap = (v->data == NULL) ? 8 : v->cap * 2;

    void *tmp = realloc(v->data, v->element_sz * new_cap);
    if (!tmp) return false;

    v->data = tmp;
    v->cap = new_cap;
  }

  uint8_t *data = (uint8_t *)v->data;

  // Shift elements to the right.
  memmove(
    data + (index + 1) * v->element_sz,
    data + index * v->element_sz,
    (v->len - index) * v->element_sz
  );

  // Insert the new element.
  memcpy(
    data + index * v->element_sz,
    value,
    v->element_sz
  );

  v->len++;
  return true;
}

void *vector_get_elem(struct Vector *v, size_t index) {
  if (!v || index >= v->len) return NULL;
  return ((uint8_t *)v->data + index * v->element_sz);
}

void *vector_get_data(struct Vector *v) {
  if (!v) return NULL;
  return v->data;
}

void vector_free(struct Vector *v) {
  if (!v) return;

  free(v->data);
  free(v);
}
