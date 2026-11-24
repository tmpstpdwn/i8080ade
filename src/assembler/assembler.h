#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "util/vector.h"

struct Vector *assemble_source(const char *file_text, const char *file_name, struct Vector *buffer);

#endif
