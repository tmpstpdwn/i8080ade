#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

#define LINE_BUF_START_SZ 64

int readline(FILE *fp, char **buf, size_t *buf_len) {
  if (!fp || !buf || !buf_len) return -2;

  int buf_i = 0;

  int ch;
  while((ch = fgetc(fp)) != EOF) {
    if (buf_i + 1 >= *buf_len) {
        size_t new_len = (*buf_len == 0)? LINE_BUF_START_SZ: *buf_len * 2;
        char *tmp = realloc(*buf, sizeof(char) * new_len);
        if (!tmp) return -2;
        *buf = tmp;
        *buf_len = new_len;
    }

    if (ch == '\n') break;
    (*buf)[buf_i++] = ch;
  }

  if (buf_i == 0 && ch == EOF) return -1;
  (*buf)[buf_i] = '\0';

  return buf_i;
}
