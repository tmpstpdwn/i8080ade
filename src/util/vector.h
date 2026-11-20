#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdbool.h>

struct Vector;

struct Vector *vector_new(size_t element_sz);
size_t vector_len(struct Vector *v);
bool vector_append(struct Vector *v, const void *value);
void *vector_get_elem(struct Vector *v, size_t index);
void *vector_get_data(struct Vector *v);
void vector_free(struct Vector *v);

#endif
