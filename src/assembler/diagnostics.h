#ifndef DIAGNOSTICS_H
#define DIAGNOSTICS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "lexer.h"
#include "util/slice.h"

#include "source.h"

#define DIAG_LIST(X) \
  /* ---- lexer ---- */ \
  X(LEX_UNRECOGNISED,      "unrecognised character '{name}'") \
  /* ---- labels ---- */ \
  X(LABEL_DUPLICATE,       "label '{name}' is already defined") \
  X(LABEL_UNDEFINED,       "undefined label '{name}'") \
  X(LABEL_IS_MNEMONIC,     "'{name}' is an instruction mnemonic and can't be used as a label") \
  X(LABEL_IS_REGISTER,     "'{name}' is a register name and can't be used as a label") \
  X(LABEL_BAD_NAME,        "'{name}' is not a valid label name: labels must start with a letter or '_'") \
  /* ---- parser ---- */ \
  X(EXPECTED_MNEMONIC,     "expected an instruction mnemonic, found '{name}'") \
  X(INCOMPLETE_INSTR,      "incomplete instruction, expected more operands") \
  X(EXPECTED_COMMA,        "expected ',' between operands, found '{name}'") \
  X(BAD_NUMBER,            "malformed number literal '{name}'") \
  X(EXPECTED_NUM,          "expected a number, found '{name}'") \
  X(EXPECTED_NUM_OR_LABEL, "expected a number or a label, found '{name}'") \
  X(EXPECTED_REGISTER,     "expected a register, found '{name}'") \
  X(TRAILING_TOKENS,       "unexpected '{name}' after the end of the instruction") \
  /* ---- validation ---- */ \
  X(MOV_M_M,               "'MOV M, M' is not a valid instruction (that encoding is HLT)") \
  X(VALUE_RANGE,           "{num} does not fit in {bits} bits (maximum {bits})") \
  X(ADDR_OVERFLOW,         "program exceeds 64KB address space (at byte {num})")

enum DiagId {
#define X(id, fmt) DIAG_##id,
  DIAG_LIST(X)
#undef X
  DIAG_COUNT
};

struct DiagArgs {
  struct Slice name;
  uint64_t num;
  uint64_t bits;
};

struct Diagnostic {
  enum DiagId id;
  struct SourceRange range;
  struct DiagArgs args;
};

bool diagnostics_add(struct SourceFile *source_file, enum DiagId id, struct SourceRange range, struct DiagArgs args);
void diagnostics_print(struct SourceFile *source_file);

#endif
