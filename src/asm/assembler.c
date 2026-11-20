#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// #include "assembler.h"

#include "util/vector.h"

#define RED   "\033[31m"
#define RESET "\033[0m"

struct LineOffset {
  size_t offset;
  size_t len;
};

struct SourceFile {
  char *text;
  const char *name;
  struct Vector *line_offsets;
  struct Vector *labels;
  struct Vector* tokens;
  struct Vector* diagnostics;
  struct Vector* parsed_items;
  struct Vector* buffer;
};

struct Diagnostic {
  char *text;
  struct SourceRange range;
};

struct OpcodeRec {
  char *name;
  enum EncType enc_t;
  uint8_t opcode;
};

static int register_code(const char *reg, enum EncType enc_t) {
  if (!reg) return -1;

  int reg_len = strlen(reg);
  if (reg_len == 0 || reg_len > 3) return -1;

  char reg_upper[4] = {0};
  int i = 0;
  while (reg[i] && i < 3) {
    reg_upper[i] = toupper(reg[i]);
    i++;
  }
  reg_upper[i] = '\0';

  int n = -1;

  if (reg_len == 1) {
    switch (reg_upper[0]) {
      case 'B':
        n = 0; break;
      case 'C':
        n = 1; break;
      case 'D':
        n = 2; break;
      case 'E':
        n = 3; break;
      case 'H':
        n = 4; break;
      case 'L':
        n = 5; break;
      case 'M':
        n = 6; break;
      case 'A':
        n = 7; break;
      default: break;
    }
  }

  switch (enc_t) {
    case ENC_REG_REG:
    case ENC_REG_ID:
    case ENC_REG_ARITH:
    case ENC_REG_IMM8:
        return n;

    case ENC_REG_PAIR_BD: {
        if (n >= 0 && n <= 2 && n % 2 == 0)
          return n / 2;
        return -1;
    }

    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_SP_IMM16:
    case ENC_REG_PAIR_PSW: {
        if (n >= 0 && n <= 4 && n % 2 == 0) {
          return n / 2;
        } else {
          char *s = (enc_t == ENC_REG_PAIR_PSW)? "PSW": "SP";
          if (strcmp(reg_upper, s) == 0) return 3;
        }
        return -1;
    }

    default:
      return -1;
  }
}

static uint32_t assemble_instr(struct ParsedInstr *parsed_instr) {
  uint32_t asmbld_instr = 0;

  enum EncType enc_t = parsed_instr->enc_t;
  uint8_t opcode = parsed_instr->opcode;

  uint8_t register1_c = parsed_instr->reg1.value;
  uint8_t register2_c = parsed_instr->reg2.value;

  uint16_t number = parsed_instr->number.value;

  switch (enc_t) {
    case ENC_NONE:
      break;

    case ENC_FIXED:
      asmbld_instr = opcode;
      break;

    case ENC_REG_REG:
      asmbld_instr = opcode | (register1_c << 3) | register2_c;
      break;

    case ENC_REG_ID:
      asmbld_instr = opcode | (register1_c << 3);
      break;

    case ENC_REG_ARITH:
      asmbld_instr = opcode | register1_c;
      break;

    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      asmbld_instr = opcode | (register1_c << 4);
      break;

    case ENC_REG_PAIR_SP_IMM16:
      asmbld_instr = number << 8 | (opcode + (register1_c << 4));
      break;

    case ENC_REG_IMM8:
      asmbld_instr = number << 8 | (opcode + (register1_c << 3));
      break;

    case ENC_IMM8:
      asmbld_instr = number << 8 | opcode;
      break;

    case ENC_ADDR16:
      asmbld_instr = number << 8 | opcode;
      break;

    case ENC_RST:
      asmbld_instr = opcode | (number << 3);
      break;
  }

  return asmbld_instr;
}

static bool assemble(struct SourceFile *source_file) {
  for (int i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);

    switch (p->type) {
      case PARSED_INSTR: {
        uint8_t size = opcode_instr_size(p->instr.enc_t);
        uint32_t assembled_instr = assemble_instr(&p->instr);
        while (size) {
          uint8_t b = assembled_instr & 0xFF; 
          if (!vector_append(source_file->buffer, &b)) return false;
          assembled_instr = assembled_instr >> 8;
          size--;
        }
        break;
      }
      default:
        break;
    }

  }
  return true;
}

static struct SourceFile *source_file_new(const char *file_name, char *file_text) {
  if (!file_text || !file_name) return NULL;

  struct SourceFile *source_file = malloc(sizeof(struct SourceFile));

  struct Vector *line_offsets = vector_new(sizeof(struct LineOffset));
  struct Vector *labels = vector_new(sizeof(struct Label));
  struct Vector *errors = vector_new(sizeof(struct Diagnostic));
  struct Vector *tokens = vector_new(sizeof(struct Token));
  struct Vector *parsed_items = vector_new(sizeof(struct ParsedItem));
  struct Vector *buffer = vector_new(sizeof(uint8_t));

  if (!source_file || !line_offsets || !errors ||
      !tokens || !parsed_items || !buffer) goto fail;

  for (size_t i = 0, l = 0; ; i++) {
    if (file_text[i] == '\n' || file_text[i] == '\0') {

      struct LineOffset lo = {
        .offset = l,
        .len = i - l
      };

      if (!vector_append(line_offsets, &lo)) goto fail;
      l = i + 1;

      if (file_text[i] == '\0') break;
    }
  }

  source_file->text = file_text;
  source_file->name = file_name;
  source_file->line_offsets = line_offsets;
  source_file->labels = labels;
  source_file->diagnostics = errors;
  source_file->tokens = tokens;
  source_file->parsed_items = parsed_items;
  source_file->buffer = buffer;

  return source_file;

fail:
  free(source_file);
  vector_free(line_offsets);
  vector_free(labels);
  vector_free(errors);
  vector_free(tokens);
  vector_free(parsed_items);

  return NULL;
}

static void source_file_free(struct SourceFile *source_file) {
  vector_free(source_file->line_offsets);
  vector_free(source_file->diagnostics);
  vector_free(source_file->labels);

  // Free token strings.
  for (int i = 0; i < vector_len(source_file->tokens); i++) {
    struct Token *token = vector_get_elem(source_file->tokens, i);
    free(token->text);
  }

  vector_free(source_file->tokens);
  vector_free(source_file->parsed_items);
  vector_free(source_file->buffer);

  free(source_file->text);
  free(source_file);
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
  if (argc != 3) {
    printf("expected \"%s <file.asm-to-read> <file.bin-to-write>\"\n", argv[0]);
    return 1;
  }

  const char *file_read_name = argv[1];
  const char *file_write_name = argv[2];

  char *file_text = get_file_text(file_read_name);
  if (!file_text) {
    printf("Reading \'%s\' into a buffer failed", file_read_name);
    return 1;
  }

  struct SourceFile *source_file = source_file_new(file_read_name, file_text);

  lexer_tokenize(source_file);
  parser_parse(source_file);
  parser_validate(source_file);
  labels_build_table(source_file);
  labels_resolve_all(source_file);
  diagnostics_print(source_file);

  if (vector_len(source_file->diagnostics) == 0) {
    assemble(source_file);
    uint8_t *buffer_data = vector_get_data(source_file->buffer);
    size_t buffer_len = vector_len(source_file->buffer);

    FILE *fpw = fopen(file_write_name, "wb");
    if (fpw && buffer_len) {
      size_t written = fwrite(buffer_data, sizeof(uint8_t), buffer_len, fpw);
      if (written != buffer_len) fprintf(stderr, "Writing buffer to \'%s\' failed\n", file_write_name);
      else printf("Written to %s\n", file_write_name);
      fclose(fpw);
    }
  }

  source_file_free(source_file);
  return 0;
}
