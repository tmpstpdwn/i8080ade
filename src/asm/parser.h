enum OperandType {
  OP_VALUE,
  OP_LABEL
};

enum ParsedItemType {
  PARSED_NONE,
  PARSED_INSTR,
  PARSED_LABEL
};

struct Operand {
  enum OperandType type;
  struct SourceRange range;
  union {
    uint64_t value;
    char *label;
  };
};

struct ParsedLabel {
  char *text;
  struct SourceRange range;
};

struct ParsedInstr {
  enum EncType enc_t;
  uint8_t opcode;
  struct Operand reg1;
  struct Operand reg2;
  struct Operand number;
};

struct ParsedItem {
  enum ParsedItemType type;
  union {
    struct ParsedInstr instr;
    struct ParsedLabel label;
  };
};
