#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

uint32_t assemble(const char *line, size_t lineno);

#endif
