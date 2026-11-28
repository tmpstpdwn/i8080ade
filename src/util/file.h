#ifndef FILE_H
#define FILE_H

#include <stddef.h>

char *file_read(const char *path);
int file_write(const char *path, const void *data, size_t len);

#endif
