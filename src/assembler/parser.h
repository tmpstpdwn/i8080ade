#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "i8080/i8080.h"
#include "lexer.h"
#include "util/slice.h"

enum OperandType {
  OP_VALUE,
  OP_LABEL
};

enum ParsedItemType {
  PARSED_NONE,
  PARSED_INSTR,
  PARSED_LABEL
};

struct Operand {
  enum OperandType type;
  struct SourceRange range;
  union {
    uint64_t value;
    struct Slice label;
  };
};

struct ParsedLabel {
  struct Slice text;
  struct SourceRange range;
};

struct ParsedInstr {
  enum EncType enc_t;
  uint8_t opcode;
  struct SourceRange range;
  struct Operand reg1;
  struct Operand reg2;
  struct Operand number;
};

struct ParsedItem {
  enum ParsedItemType type;
  union {
    struct ParsedInstr instr;
    struct ParsedLabel label;
  };
};

void parser_parse(struct SourceFile *source_file);
void parser_validate(struct SourceFile *source_file);
void parser_print_items(struct SourceFile *source_file);

#endif
