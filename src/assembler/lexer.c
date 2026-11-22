#include <stddef.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#include "util/vector.h"
#include "lexer.h"
#include "diagnostics.h"
#include "assembler.h"

// Returns a malloc'ed, null terminated string which must be free'd.
static char *substr(const char *str, size_t from, size_t to) {
  if (!str || from >= to) return NULL;

  size_t str_len = strlen(str);

  if (from >= str_len || to > str_len) return NULL;
  
  size_t substr_len = to - from;
  char *substr = malloc(sizeof(char) * substr_len + 1);
  if (!substr) return NULL;

  for (size_t i = 0; i < substr_len; i++)
    substr[i] = str[from + i];

  substr[substr_len] = '\0';

  return substr;
}

static bool add_token(struct SourceFile *source_file, enum TokenKind kind,
                 size_t line, size_t from, size_t to) {
  struct SourceRange range = {line, from, to};
  char *str = NULL;

  if (kind != TK_NEWLINE) {
    str = substr(source_file->text, from, to);
    if (!str) return false;
  }

  if (kind == TK_INVALID) {
    diagnostics_add(source_file, (struct Diagnostic){"Unrecognised token", range});
  }

  struct Token token = (struct Token){kind, str, range};

  if (!vector_append(source_file->tokens, &token)) {
    free(str);
    return false;
  }

  return true;
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
    struct Token *token = vector_get_elem(source_file->tokens, i);
    switch (token->kind) {
      case TK_IDENT:
        printf("IDENT: %s\n", token->text);
        break;
      case TK_NONE:
        printf("NONE\n");
        break;
      case TK_INVALID:
        printf("INVALID: %s\n", token->text);
        break;
      case TK_PUNCT:
        printf("PUNCT %s\n", token->text);
        break;
      case TK_NEWLINE:
        printf("NEWLINE\n");
        break;
      case TK_NUM:
        printf("NUM: %s\n", token->text);
        break;
    }
  }
}
