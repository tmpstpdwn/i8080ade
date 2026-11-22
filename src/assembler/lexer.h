#ifndef LEXER_H
#define LEXER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct SourceFile;

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
  struct SourceRange range;
};

bool lexer_tokenize(struct SourceFile *source_file);
struct Token *lexer_consume_token(struct SourceFile *source_file, size_t n);
struct Token *lexer_peek_token(struct SourceFile *source_file, size_t n);
void lexer_print_tokens(struct SourceFile *source_file);

#endif
