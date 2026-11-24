#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "util/vector.h"
#include "util/slice.h"
#include "lexer.h"
#include "diagnostics.h"

#include "source.h"

static bool add_token(struct SourceFile *source_file, enum TokenKind kind,
                      size_t line, size_t from, size_t to) {
  if (from > to) return false;

  struct SourceRange range = {line, from, to};
  struct Slice slice = {source_file->text + from, to - from};

  if (kind == TK_INVALID)
    diagnostics_add(source_file, (struct Diagnostic){"Unrecognised token", range});

  struct Token token = {kind, slice, range};

  return vector_append(source_file->tokens, &token);
}

bool lexer_tokenize(struct SourceFile *source_file) {
  if (!source_file) return false;

  const char *s = source_file->text;
  size_t i = 0, line = 0;
  enum TokenKind prev = TK_NONE;

  for (;;) {
    while (s[i] == ' ' || s[i] == '\t' || s[i] == '\r') i++;
    if (s[i] == ';')
      while (s[i] && s[i] != '\n') i++;

    unsigned char c = (unsigned char)s[i];
    if (c == '\0') break;

    size_t from = i;
    enum TokenKind kind;

    if (c == '\n') {
      i++;
      if (prev != TK_NONE && prev != TK_NEWLINE) {
        if (!add_token(source_file, TK_NEWLINE, line, from, i)) return false;
        prev = TK_NEWLINE;
      }
      line++;
      continue;
    }

    if (isdigit(c)) {
      while (isalnum((unsigned char)s[i])) i++;
      kind = TK_NUM;
    } else if (isalpha(c) || c == '_') {
      while (isalnum((unsigned char)s[i]) || s[i] == '_') i++;
      kind = TK_IDENT;
    } else if (c == ',' || c == ':') {
      i++;
      kind = TK_PUNCT;
    } else {
      i++;
      kind = TK_INVALID;
    }

    if (!add_token(source_file, kind, line, from, i)) return false;
    prev = kind;
  }

  if (prev != TK_NONE && prev != TK_NEWLINE)
    add_token(source_file, TK_NEWLINE, line, 0, 0);
  return true;
}

static struct Token* lexer_get_token(struct SourceFile *source_file, bool consume, size_t n) {
  if (!source_file || n == 0) return NULL;

  struct Token *ret_token = NULL;

  size_t step = n - 1;
  size_t tokens_len = vector_len(source_file->tokens);
  if (source_file->token_cursor + step < tokens_len) {
    ret_token = vector_get_elem(source_file->tokens, source_file->token_cursor + step);
    if (consume) {
      source_file->token_cursor += n;      
    }
  }

  return ret_token;
}

struct Token *lexer_consume_token(struct SourceFile *source_file, size_t n) {
  return lexer_get_token(source_file, true, n);
}

struct Token *lexer_peek_token(struct SourceFile *source_file, size_t n) {
  return lexer_get_token(source_file, false, n);
}

void lexer_print_tokens(struct SourceFile *source_file) {
  for (size_t i = 0; i < vector_len(source_file->tokens); i++) {
    const struct Token *token = vector_get_elem(source_file->tokens, i);
    int len = (int)token->slice.len;
    const char *text = token->slice.text;

    switch (token->kind) {
      case TK_IDENT:
        printf("IDENT: %.*s\n", len, text);
        break;
      case TK_NONE:
        printf("NONE\n");
        break;
      case TK_INVALID:
        printf("INVALID: %.*s\n", len, text);
        break;
      case TK_PUNCT:
        printf("PUNCT: %.*s\n", len, text);
        break;
      case TK_NEWLINE:
        printf("NEWLINE\n");
        break;
      case TK_NUM:
        printf("NUM: %.*s\n", len, text);
        break;
    }
  }
}
