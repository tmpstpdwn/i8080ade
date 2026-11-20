#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "util/vector.h"
#include "asm/assembler.h"
#include "disasm/disassembler.h"

static void print_usage(const char *program) {
  printf("Usage:\n");
  printf("  %s -a <input.asm> <output.bin>\n", program);
  printf("  %s -d <input.bin>\n", program);
  printf("  %s -h | --help\n", program);

  printf("\nOptions:\n");
  printf("  -a    Assemble a source file\n");
  printf("  -d    Disassemble a binary file\n");
  printf("  -h    Display this help message\n");
}

static size_t get_file_size(FILE *fp) {
  size_t file_size = 0;
  if (fp) {
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);
  }
  return file_size;
}

static char *get_file_text(const char *read_from) {
  FILE *fp = fopen(read_from, "rb");
  char *text = NULL;

  if (!fp) return NULL;

  size_t file_size = get_file_size(fp);
  if (!file_size) goto end;

  text = malloc((size_t)file_size + 1);

  if (!text) goto end;

  size_t bytes_read = fread(text, 1, (size_t)file_size, fp);
  text[bytes_read] = '\0';

end:
  fclose(fp);
  return text;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    print_usage(argv[0]);
    return 1;
  }

  if (strcmp(argv[1], "-h") == 0 ||
      strcmp(argv[1], "--help") == 0) {
    print_usage(argv[0]);
    return 0;
  }

  if (strcmp(argv[1], "-a") == 0) {
    if (argc != 4) {
      fprintf(stderr, "Error: Invalid arguments for -a\n");
      print_usage(argv[0]);
      return 1;
    }

    const char *input_path = argv[2];
    const char *output_path = argv[3];

    char *file_text = get_file_text(input_path);
    if (!file_text) {
      fprintf(stderr, "Error: Reading '%s' into a buffer failed\n", input_path);
      return 1;
    }

    struct Vector *buffer = vector_new(sizeof(uint8_t));
    if (!buffer) {
      fprintf(stderr, "Error: Memory allocation failed for buffer\n");
      free(file_text);
      return 1;
    }

    struct Vector *res = assemble_source(file_text, input_path, buffer);

    if (!res) {
      fprintf(stderr, "Error: Assembly failed\n");
      vector_free(buffer);
      return 1;
    }

    FILE *fpw = fopen(output_path, "wb");
    if (!fpw) {
      fprintf(stderr, "Error: Could not open '%s' for writing\n", output_path);
      vector_free(buffer);
      return 1;
    }

    size_t buffer_len = vector_len(buffer);
    size_t written = fwrite(vector_get_data(buffer), sizeof(uint8_t), buffer_len, fpw);
    fclose(fpw);

    if (written != buffer_len) {
      fprintf(stderr, "Error: Writing buffer to '%s' failed\n", output_path);
      vector_free(buffer);
      return 1;
    }

    printf("Written to %s\n", output_path);
    vector_free(buffer);
    return 0;
  }

  else if (strcmp(argv[1], "-d") == 0) {
    if (argc != 3) {
      fprintf(stderr, "Error: Invalid arguments for -d\n");
      print_usage(argv[0]);
      return 1;
    }

    return disassemble_rom(argv[2]) ? 0 : 1;
  }

  fprintf(stderr, "Error: Unknown option '%s'\n", argv[1]);
  print_usage(argv[0]);

  return 1;
}
