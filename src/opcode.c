static const struct OpcodeRec first_token[] = {
  {"NOP",       ENC_FIXED,                  0x00},
  {"LXI",       ENC_REG_PAIR_SP_IMM16,      0x01},
  {"STAX",      ENC_REG_PAIR_BD,            0x02},
  {"INX",       ENC_REG_PAIR_SP,            0x03},
  {"INR",       ENC_REG_ID,                 0x04},
  {"DCR",       ENC_REG_ID,                 0x05},
  {"MVI",       ENC_REG_IMM8,               0x06},
  {"RLC",       ENC_FIXED,                  0x07},
  {"DAD",       ENC_REG_PAIR_SP,            0x09},
  {"LDAX",      ENC_REG_PAIR_BD,            0x0A},
  {"DCX",       ENC_REG_PAIR_SP,            0x0B},
  {"RRC",       ENC_FIXED,                  0x0F},
  {"RAL",       ENC_FIXED,                  0x17},
  {"RAR",       ENC_FIXED,                  0x1F},
  {"SHLD",      ENC_ADDR16,                 0x22},
  {"DAA",       ENC_FIXED,                  0x27},
  {"LHLD",      ENC_ADDR16,                 0x2A},
  {"CMA",       ENC_FIXED,                  0x2F},
  {"STA",       ENC_ADDR16,                 0x32},
  {"STC",       ENC_FIXED,                  0x37},
  {"LDA",       ENC_ADDR16,                 0x3a},
  {"CMC",       ENC_FIXED,                  0x3F},
  {"MOV",       ENC_REG_REG,                0x40},
  {"HLT",       ENC_FIXED,                  0x76},
  {"ADD",       ENC_REG_ARITH,              0x80},
  {"ADC",       ENC_REG_ARITH,              0x88},
  {"SUB",       ENC_REG_ARITH,              0x90},
  {"SBB",       ENC_REG_ARITH,              0x98},
  {"ANA",       ENC_REG_ARITH,              0xA0},
  {"XRA",       ENC_REG_ARITH,              0xA8},
  {"ORA",       ENC_REG_ARITH,              0xB0},
  {"CMP",       ENC_REG_ARITH,              0xB8},
  {"RNZ",       ENC_FIXED,                  0xC0},
  {"POP",       ENC_REG_PAIR_PSW,           0xC1},
  {"JNZ",       ENC_ADDR16,                 0xC2},
  {"JMP",       ENC_ADDR16,                 0xC3},
  {"CNZ",       ENC_ADDR16,                 0xC4},
  {"PUSH",      ENC_REG_PAIR_PSW,           0xc5},
  {"ADI",       ENC_IMM8,                   0xC6},
  {"RST",       ENC_RST,                    0xC7},
  {"RZ",        ENC_FIXED,                  0xC8},
  {"RET",       ENC_FIXED,                  0xC9},
  {"JZ",        ENC_ADDR16,                 0xCA},
  {"CZ",        ENC_ADDR16,                 0xCC},
  {"CALL",      ENC_ADDR16,                 0xCD},
  {"ACI",       ENC_IMM8,                   0xCE},
  {"RNC",       ENC_FIXED,                  0xD0},
  {"JNC",       ENC_ADDR16,                 0xD2},
  {"OUT",       ENC_IMM8,                   0xD3},
  {"CNC",       ENC_ADDR16,                 0xD4},
  {"SUI",       ENC_IMM8,                   0xD6},
  {"RC",        ENC_FIXED,                  0xD8},
  {"JC",        ENC_ADDR16,                 0xDA},
  {"IN",        ENC_IMM8,                   0xDB},
  {"CC",        ENC_ADDR16,                 0xDC},
  {"SBI",       ENC_IMM8,                   0xDE},
  {"RPO",       ENC_FIXED,                  0xE0},
  {"JPO",       ENC_ADDR16,                 0xE2},
  {"XTHL",      ENC_FIXED,                  0xE3},
  {"CPO",       ENC_ADDR16,                 0xE4},
  {"ANI",       ENC_IMM8,                   0xE6},
  {"RPE",       ENC_FIXED,                  0xE8},
  {"PCHL",      ENC_FIXED,                  0xE9},
  {"JPE",       ENC_ADDR16,                 0xEA},
  {"XCHG",      ENC_FIXED,                  0xEB},
  {"CPE",       ENC_ADDR16,                 0xEC},
  {"XRI",       ENC_IMM8,                   0xEE},
  {"RP",        ENC_FIXED,                  0xF0},
  {"JP",        ENC_ADDR16,                 0xF2},
  {"DI",        ENC_FIXED,                  0xF3},
  {"CP",        ENC_ADDR16,                 0xF4},
  {"ORI",       ENC_IMM8,                   0xF6},
  {"RM",        ENC_FIXED,                  0xF8},
  {"SPHL",      ENC_FIXED,                  0xF9},
  {"JM",        ENC_ADDR16,                 0xFA},
  {"EI",        ENC_FIXED,                  0xFB},
  {"CM",        ENC_ADDR16,                 0xFC},
  {"CPI",       ENC_IMM8,                   0xFE},
  {NULL}
};

static bool oprec_get(const char *mnemonic, struct OpcodeRec *oprec) {
  if (!mnemonic) return false;

  int mnemonic_len = strlen(mnemonic);
  if (mnemonic_len == 0 || mnemonic_len > 4) return false;

  char mnemonic_upper[5] = {0};
  int i = 0;
  while (mnemonic[i] && i < 4) {
    mnemonic_upper[i] = toupper(mnemonic[i]);    
    i++;
  }
  mnemonic_upper[i] = '\0';

  for (int i = 0; first_token[i].name != NULL; i++) {
    if (strcmp(mnemonic_upper, first_token[i].name) == 0) {
      *oprec = first_token[i];
      return true;
    }
  }

  return false;
}

static uint8_t opcode_instr_size(enum EncType enc_t) {
  switch (enc_t) {
    case ENC_NONE:
      return 0;

    case ENC_RST:
    case ENC_FIXED:
    case ENC_REG_ID:
    case ENC_REG_REG:
    case ENC_REG_ARITH:
    case ENC_REG_PAIR_SP:
    case ENC_REG_PAIR_BD:
    case ENC_REG_PAIR_PSW:
      return 1;

    case ENC_IMM8:
    case ENC_REG_IMM8:
      return 2;

    case ENC_ADDR16:
    case ENC_REG_PAIR_SP_IMM16:
      return 3;
  }
}
