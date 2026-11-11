#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

uint32_t assemble(const char *line, size_t lineno);
bool assemble_rom(char *read_from, char *write_to);

#endif
