#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <stdint.h>
#include <stddef.h>

size_t disassemble(uint8_t *buff, size_t pc);

#endif
