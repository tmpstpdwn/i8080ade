enum SyntaxKind {
  SK_OPCODE,
  SK_COMMA,
  SK_REG,
  SK_D8,
  SK_D16,
  SK_A16,
  SK_END
};

static const enum SyntaxKind syntax_table[][5] = {
  [ENC_FIXED] = {SK_OPCODE, SK_END},
  [ENC_REG_REG] = {SK_OPCODE, SK_REG, SK_COMMA, SK_REG, SK_END},
  [ENC_REG_ID] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_ARITH] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_SP] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_BD] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_PSW] = {SK_OPCODE, SK_REG, SK_END},
  [ENC_REG_PAIR_SP_IMM16] = {SK_OPCODE, SK_REG, SK_COMMA, SK_D16, SK_END},
  [ENC_REG_IMM8] = {SK_OPCODE, SK_REG, SK_COMMA, SK_D8, SK_END},
  [ENC_IMM8] = {SK_OPCODE, SK_D8, SK_END},
  [ENC_ADDR16] = {SK_OPCODE, SK_A16, SK_END},
  [ENC_RST] = {SK_OPCODE, SK_D8, SK_END},
};

static bool parser_add_item(struct SourceFile *source_file, struct ParsedItem parsed_item) {
  if (!source_file) return false;
  return vector_append(source_file->parsed_items, &parsed_item);
}

static bool parser_parse_line(struct SourceFile *source_file) {
  if (!source_file) return false;

  struct Token *first_token = lexer_peek_token(source_file, 1);
  struct Token *second_token = lexer_peek_token(source_file, 2);

  if (first_token && first_token->kind == TK_IDENT) {
    if (second_token && second_token->kind == TK_PUNCT && second_token->text[0] == ':') {
      struct ParsedLabel label = {first_token->text};
      parsed_items_add(source_file, (struct ParsedItem){.type = PARSED_LABEL, .label = label});
      lexer_consume_token(source_file, 2);
    }
  }

  if (lexer_peek_token(source_file, 1) && lexer_peek_token(source_file, 1)->kind == TK_NEWLINE) {
    lexer_consume_token(source_file, 1);    
    return true;
  }

  struct OpcodeRec oprec;
  const enum SyntaxKind *syntax_array = NULL;
  size_t syntax_arr_i = 0;
  struct ParsedInstr parsed_instr = {ENC_NONE};

  while (true) {
    struct Token *token = lexer_consume_token(source_file, 1);
    if (!token) return false;

    if (token->kind == TK_INVALID) goto fail;

    if (syntax_array == NULL) {
      if (token->kind == TK_IDENT && get_oprec(token->text, &oprec)) {
          syntax_array = syntax_table[oprec.enc_t];
        } else {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a valid opcode mnemonic", token->range});
          goto fail;
        }
    }

    if (token->kind == TK_NEWLINE && syntax_array[syntax_arr_i] != SK_END) {
      diagnostics_add(source_file, (struct Diagnostic){"Incomplete instruction, expected more operands", token->range});
      goto fail;
    }

    switch (syntax_array[syntax_arr_i]) {
      case SK_OPCODE:
        parsed_instr.enc_t = oprec.enc_t;
        parsed_instr.opcode = oprec.opcode;
        break;

      case SK_COMMA:
        if (token->kind != TK_PUNCT || token->text[0] != ',') {
          diagnostics_add(source_file, (struct Diagnostic){"Expected \',\' between operands", token->range});
          goto fail;
        }
        break;

      case SK_D8:
      case SK_D16:
      case SK_A16:
        if (token->kind != TK_NUM) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a number", token->range});
          goto fail;
        }
        parsed_instr.number.type = OP_VALUE;
        parsed_instr.number.range = token->range;
        parsed_instr.number.value = token->value;
        break;

      case SK_REG: {
        int register_c = register_code(token->text, oprec.enc_t);
        if (register_c == -1) {
          diagnostics_add(source_file, (struct Diagnostic){"Expected a register", token->range});
          goto fail;
        }

        if (syntax_arr_i == 1) {
          parsed_instr.reg1.type = OP_VALUE;
          parsed_instr.reg1.range = token->range;
          parsed_instr.reg1.value = register_c;
        } else {
          parsed_instr.reg2.type = OP_VALUE;
          parsed_instr.reg2.range = token->range;
          parsed_instr.reg2.value = register_c;          
        }
        break;
      }

      case SK_END:
        if (token->kind != TK_NEWLINE) {
          diagnostics_add(source_file, (struct Diagnostic){"Unexpected trailing tokens", token->range});
          goto fail;
        }
        parsed_items_add(source_file, (struct ParsedItem){.type = PARSED_INSTR, .instr = parsed_instr});
        return true;
    }

  syntax_arr_i++;
  }

  return true;

fail:
  while (lexer_peek_token(source_file, 1) && lexer_peek_token(source_file, 1)->kind != TK_NEWLINE)
    lexer_consume_token(source_file, 1); // Consume everything till newline.
  lexer_consume_token(source_file, 1); // Consume newline.
  return true;
}

static void parser_parse(struct SourceFile *source_file) {
  while (parser_parse_line(source_file));
}

static void parser_validate(struct SourceFile *source_file) {
  for (int i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    if (p->type != PARSED_INSTR) return; 

    switch (p->instr.enc_t) {
      case ENC_REG_REG:
        if (p->instr.reg1.value == 6 && p->instr.reg2.value == 6) {
          struct SourceRange range = {p->instr.reg1.range.lineno, p->instr.reg1.range.start_offset, p->instr.reg2.range.end_offset};
          diagnostics_add(source_file, (struct Diagnostic){"Invalid Operands", range});
        }
        break;
      case ENC_ADDR16:
      case ENC_REG_PAIR_SP_IMM16:
        if (p->instr.number.value > 0xFFFF)
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0000 - 0xFFFF (16-bit)", p->instr.number.range});
        break;
      case ENC_IMM8:
      case ENC_REG_IMM8:
        if (p->instr.number.value > 0xFF)
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x00 - 0xFF (8-bit)", p->instr.number.range});
        break;
      case ENC_RST:
        if (p->instr.number.value >= 8)
          diagnostics_add(source_file, (struct Diagnostic){"Expected a value within the range 0x0 - 0x7 (3-bit)", p->instr.number.range});
        break;
      default: break;
    }

  }
}

static void parser_print_items(struct SourceFile *source_file) {
  const char *enc_str[] = {
    "ENC_NONE", "ENC_FIXED", "ENC_REG_REG", "ENC_REG_ID",
    "ENC_REG_ARITH", "ENC_REG_PAIR_SP", "ENC_REG_PAIR_BD",
    "ENC_REG_PAIR_PSW", "ENC_REG_PAIR_SP_IMM16", "ENC_REG_IMM8",
    "ENC_IMM8", "ENC_ADDR16", "ENC_RST"
  };

  for (int i = 0; i < vector_len(source_file->parsed_items); i++) {
    struct ParsedItem *p = vector_get_elem(source_file->parsed_items, i);
    switch (p->type) {
      case PARSED_NONE:
        break;
      case PARSED_LABEL:
        printf("LABEL:\n");
        printf("      %s\n", p->label.text);
        break;
      case PARSED_INSTR:
        printf("INSTR:\n");
        printf("      enc : %s\n", enc_str[p->instr.enc_t]);
        printf("      op  : 0x%x\n", p->instr.opcode);

        printf("      num : ");
        switch (p->instr.number.type) {
          case OP_LABEL:
            printf("%s (unresolved)\n", p->instr.number.label);
            break;
          case OP_VALUE:
            printf("0x%x\n", p->instr.number.value);
            break;
        }

        printf("      r1  : %d\n", p->instr.reg1.value);
        printf("      r2  : %d\n", p->instr.reg2.value);
        break;
    }
  }
}
