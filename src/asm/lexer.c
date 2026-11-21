#include <errno.h>
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

static bool str_to_num(const char *str, uint64_t *num) {
  if (!str || !*str || !num)
    return false;

  int base = 10;
  const char *p = str;

  if (p[0] == '0' && p[1]) {
    switch (tolower((unsigned char)p[1])) {
      case 'x': base = 16; break;
      case 'b': base = 2;  break;
      case 'o': base = 8;  break;
    }

    if (base != 10)
      p += 2;
  }

  if (!*p)
    return false;

  errno = 0;
  char *end;


  unsigned long long value = strtoull(p, &end, base);

  if (errno == ERANGE || value > UINT64_MAX)
    return false;

  if (end == p || *end != '\0')
    return false;

  *num = (uint64_t)value;
  return true;
}

// Returns a malloc'ed, null terminated string which much be free'd.
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


static bool lexer_add_token(struct SourceFile *source_file, struct Token token) {
  if (!source_file) return false;
  return vector_append(source_file->tokens, &token);
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

static enum TokenKind lexer_get_tk(enum TokenKind last_token_kind, char curr_token) {
  if (isalpha(curr_token) || curr_token == '_') {
    if (last_token_kind == TK_NUM)
      return TK_NUM;
    return TK_IDENT;
  }

  else if (isdigit(curr_token)) {
    if (last_token_kind == TK_IDENT)
      return TK_IDENT;
    return TK_NUM;
  }

  if (curr_token == ',' || curr_token == ':' || curr_token == ';') return TK_PUNCT;
  else if (curr_token == '\n') return TK_NEWLINE;
  else if (isspace(curr_token) || curr_token == '\0') return TK_NONE;

  return TK_INVALID;
}

static inline bool lexer_single_token(char c) {
  enum TokenKind tokenkind = lexer_get_tk(TK_NONE, c);
  return tokenkind == TK_PUNCT || tokenkind == TK_INVALID;
}

bool lexer_tokenize(struct SourceFile *source_file) {
  if (!source_file) return false;

  char *file_text = source_file->text;

  enum TokenKind last_tk = TK_NONE;
  enum TokenKind curr_tk = TK_NONE;

  size_t i = 0;
  size_t from = 0;
  size_t lineno = 0;

  while (true) {
    char c = file_text[i];
    curr_tk = lexer_get_tk(last_tk, c);

    // Process tokens.
    if (curr_tk != last_tk || lexer_single_token(c)) {
      if (last_tk != TK_NONE) {
        char *token_str = substr(file_text, from, i);
        if (!token_str) return false;
        struct SourceRange range = {lineno, from, i};
        uint64_t value = 0;

        if (last_tk == TK_NUM) {
          if (!str_to_num(token_str, &value))
            last_tk = TK_INVALID;
        } else if (last_tk == TK_INVALID) {
          diagnostics_add(source_file, (struct Diagnostic){"Unrecognised token", range});
        } else if (last_tk == TK_NEWLINE) {
          free(token_str);
          token_str = NULL;
          if (vector_len(source_file->tokens) == 0)
            goto skip_token_add;
        } else if (last_tk == TK_PUNCT) {
          // Skip comments.
          if (strcmp(token_str, ";") == 0) {
            while ((c = file_text[i++]) && c != '\n');
            if (c == '\n') i -= 2;
            curr_tk = TK_NONE;
            free(token_str);
            token_str = NULL;
            goto loop_end;
          }
        } 

        if (!lexer_add_token(source_file, (struct Token){last_tk, token_str, value, range})) {
          return false;
        }

      }

skip_token_add:
      from = i;
    }

loop_end:
    if (c == '\0') break;

    if (last_tk == TK_NEWLINE) lineno++;

    last_tk = curr_tk;
    i++;
  }

  if (vector_len(source_file->tokens) != 0 && last_tk != TK_NEWLINE)
    return lexer_add_token(source_file, (struct Token){TK_NEWLINE, NULL, 0, {lineno, 0, 0}});

  return true;
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
        printf("NUM: %s, %d\n", token->text, (int)token->value);
        break;
    }
  }
}
