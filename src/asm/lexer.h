#include <stdint.h>
#include <stddef.h>

enum TokenKind {
  TK_NONE,
  TK_IDENT,
  TK_PUNCT,
  TK_NUM,
  TK_NEWLINE,
  TK_INVALID,
};

struct SourceRange {
  size_t lineno;
  size_t start_offset;
  size_t end_offset;
};

struct Token {
  enum TokenKind kind;
  char *text;
  uint64_t value;
  struct SourceRange range;
};
