#include <stdio.h>
#include <stdlib.h>

char *file_read(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) return NULL;

  char *text = NULL;
  long size = -1;
  if (fseek(fp, 0, SEEK_END) == 0) size = ftell(fp);

  if (size >= 0 && fseek(fp, 0, SEEK_SET) == 0 &&
      (text = malloc((size_t)size + 1))) {
    size_t n = fread(text, 1, (size_t)size, fp);
    text[n] = '\0';
  }

  fclose(fp);
  return text;
}

int file_write(const char *path, const void *data, size_t len) {
  FILE *fp = fopen(path, "wb");
  if (!fp) return 0;
  int ok = fwrite(data, 1, len, fp) == len;
  return fclose(fp) == 0 && ok;
}
