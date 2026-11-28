#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "i8080/i8080.h"

static int total_tests = 0;
static int passed_tests = 0;

static char buf[32];

static void test_encode(const char *name, struct I8080Instr instr, uint8_t exp_sz,
                         uint8_t b0, uint8_t b1, uint8_t b2) {
  total_tests++;

  struct I8080Bytes instr_bytes = {0};

  i8080_encode_instr(instr, &instr_bytes);

  bool ok = (instr_bytes.size == exp_sz) &&
            (exp_sz < 1 || instr_bytes.bytes[0] == b0) &&
            (exp_sz < 2 || instr_bytes.bytes[1] == b1) &&
            (exp_sz < 3 || instr_bytes.bytes[2] == b2);

  if (ok) {
    passed_tests++;
  } else {
    printf(
        "FAIL: %s (got size %u: %02X %02X %02X, "
        "expected size %u: %02X %02X %02X)\n",
        name,
        instr_bytes.size, instr_bytes.bytes[0], instr_bytes.bytes[1], instr_bytes.bytes[2],
        exp_sz, b0, b1, b2
    );
  }
}

int main(void) {
  // 1. ENC_FIXED
  test_encode("NOP (0x00)", ((struct I8080Instr){.opcode = 0x00, .enc_t = ENC_FIXED, .size = 1}), 1, 0x00, 0, 0);
  test_encode("RLC (0x07)", ((struct I8080Instr){.opcode = 0x07, .enc_t = ENC_FIXED, .size = 1}), 1, 0x07, 0, 0);
  test_encode("RRC (0x0F)", ((struct I8080Instr){.opcode = 0x0F, .enc_t = ENC_FIXED, .size = 1}), 1, 0x0F, 0, 0);
  test_encode("RAL (0x17)", ((struct I8080Instr){.opcode = 0x17, .enc_t = ENC_FIXED, .size = 1}), 1, 0x17, 0, 0);
  test_encode("RAR (0x1F)", ((struct I8080Instr){.opcode = 0x1F, .enc_t = ENC_FIXED, .size = 1}), 1, 0x1F, 0, 0);
  test_encode("DAA (0x27)", ((struct I8080Instr){.opcode = 0x27, .enc_t = ENC_FIXED, .size = 1}), 1, 0x27, 0, 0);
  test_encode("CMA (0x2F)", ((struct I8080Instr){.opcode = 0x2F, .enc_t = ENC_FIXED, .size = 1}), 1, 0x2F, 0, 0);
  test_encode("STC (0x37)", ((struct I8080Instr){.opcode = 0x37, .enc_t = ENC_FIXED, .size = 1}), 1, 0x37, 0, 0);
  test_encode("CMC (0x3F)", ((struct I8080Instr){.opcode = 0x3F, .enc_t = ENC_FIXED, .size = 1}), 1, 0x3F, 0, 0);
  test_encode("HLT (0x76)", ((struct I8080Instr){.opcode = 0x76, .enc_t = ENC_FIXED, .size = 1}), 1, 0x76, 0, 0);
  test_encode("RET (0xC9)", ((struct I8080Instr){.opcode = 0xC9, .enc_t = ENC_FIXED, .size = 1}), 1, 0xC9, 0, 0);
  test_encode("RNZ (0xC0)", ((struct I8080Instr){.opcode = 0xC0, .enc_t = ENC_FIXED, .size = 1}), 1, 0xC0, 0, 0);
  test_encode("RZ  (0xC8)", ((struct I8080Instr){.opcode = 0xC8, .enc_t = ENC_FIXED, .size = 1}), 1, 0xC8, 0, 0);
  test_encode("RNC (0xD0)", ((struct I8080Instr){.opcode = 0xD0, .enc_t = ENC_FIXED, .size = 1}), 1, 0xD0, 0, 0);
  test_encode("RC  (0xD8)", ((struct I8080Instr){.opcode = 0xD8, .enc_t = ENC_FIXED, .size = 1}), 1, 0xD8, 0, 0);
  test_encode("RPO (0xE0)", ((struct I8080Instr){.opcode = 0xE0, .enc_t = ENC_FIXED, .size = 1}), 1, 0xE0, 0, 0);
  test_encode("RPE (0xE8)", ((struct I8080Instr){.opcode = 0xE8, .enc_t = ENC_FIXED, .size = 1}), 1, 0xE8, 0, 0);
  test_encode("RP  (0xF0)", ((struct I8080Instr){.opcode = 0xF0, .enc_t = ENC_FIXED, .size = 1}), 1, 0xF0, 0, 0);
  test_encode("RM  (0xF8)", ((struct I8080Instr){.opcode = 0xF8, .enc_t = ENC_FIXED, .size = 1}), 1, 0xF8, 0, 0);
  test_encode("PCHL (0xE9)", ((struct I8080Instr){.opcode = 0xE9, .enc_t = ENC_FIXED, .size = 1}), 1, 0xE9, 0, 0);
  test_encode("SPHL (0xF9)", ((struct I8080Instr){.opcode = 0xF9, .enc_t = ENC_FIXED, .size = 1}), 1, 0xF9, 0, 0);
  test_encode("XTHL (0xE3)", ((struct I8080Instr){.opcode = 0xE3, .enc_t = ENC_FIXED, .size = 1}), 1, 0xE3, 0, 0);
  test_encode("XCHG (0xEB)", ((struct I8080Instr){.opcode = 0xEB, .enc_t = ENC_FIXED, .size = 1}), 1, 0xEB, 0, 0);
  test_encode("DI   (0xF3)", ((struct I8080Instr){.opcode = 0xF3, .enc_t = ENC_FIXED, .size = 1}), 1, 0xF3, 0, 0);
  test_encode("EI   (0xFB)", ((struct I8080Instr){.opcode = 0xFB, .enc_t = ENC_FIXED, .size = 1}), 1, 0xFB, 0, 0);

  // ENC_REG_REG
  // Test all 63 valid combinations (excluding 0x76 MOV M, M)
  for (uint8_t r1 = 0; r1 < 8; r1++) {
    for (uint8_t r2 = 0; r2 < 8; r2++) {
      if (r1 == 6 && r2 == 6) continue; // HLT
      uint8_t exp = 0x40 | (r1 << 3) | r2;
      snprintf(buf, sizeof(buf), "MOV r1=%u, r2=%u", r1, r2);
      test_encode(buf, ((struct I8080Instr){.opcode = 0x40, .enc_t = ENC_REG_REG, .reg_1 = r1, .reg_2 = r2, .size = 1}), 1, exp, 0, 0);
    }
  }

  // ENC_REG_ID
  for (uint8_t reg = 0; reg < 8; reg++) {
    snprintf(buf, sizeof(buf), "INR reg=%u", reg);
    test_encode(buf, ((struct I8080Instr){.opcode = 0x04, .enc_t = ENC_REG_ID, .reg_1 = reg, .size = 1}), 1, 0x04 | (reg << 3), 0, 0);
    snprintf(buf, sizeof(buf), "DCR reg=%u", reg);
    test_encode(buf, ((struct I8080Instr){.opcode = 0x05, .enc_t = ENC_REG_ID, .reg_1 = reg, .size = 1}), 1, 0x05 | (reg << 3), 0, 0);
  }

  // ENC_REG_ARITH
  static const uint8_t arith_bases[8] = { 0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8 };
  for (int a = 0; a < 8; a++) {
    for (uint8_t reg = 0; reg < 8; reg++) {
      snprintf(buf, sizeof(buf), "ARITH op=0x%02X reg=%u", arith_bases[a], reg);
      test_encode(buf, ((struct I8080Instr){.opcode = arith_bases[a], .enc_t = ENC_REG_ARITH, .reg_1 = reg, .size = 1}), 1, arith_bases[a] | reg, 0, 0);
    }
  }

  // ENC_REG_PAIR_SP
  static const uint8_t rp_sp_ops[] = { 0x03, 0x0B, 0x09 };
  for (int o = 0; o < 3; o++) {
    for (uint8_t rp = 0; rp < 4; rp++) {
      snprintf(buf, sizeof(buf), "RP_SP op=0x%02X rp=%u", rp_sp_ops[o], rp);
      test_encode(buf, ((struct I8080Instr){.opcode = rp_sp_ops[o], .enc_t = ENC_REG_PAIR_SP, .reg_1 = rp, .size = 1}), 1, rp_sp_ops[o] | (rp << 4), 0, 0);
    }
  }

  // ENC_REG_PAIR_BD
  static const uint8_t rp_bd_ops[] = { 0x02, 0x0A };
  for (int o = 0; o < 2; o++) {
    for (uint8_t rp = 0; rp < 2; rp++) {
      snprintf(buf, sizeof(buf), "RP_BD op=0x%02X rp=%u", rp_bd_ops[o], rp);
      test_encode(buf, ((struct I8080Instr){.opcode = rp_bd_ops[o], .enc_t = ENC_REG_PAIR_BD, .reg_1 = rp, .size = 1}), 1, rp_bd_ops[o] | (rp << 4), 0, 0);
    }
  }

  // ENC_REG_PAIR_PSW
  static const uint8_t rp_psw_ops[] = { 0xC5, 0xC1 };
  for (int o = 0; o < 2; o++) {
    for (uint8_t rp = 0; rp < 4; rp++) {
      snprintf(buf, sizeof(buf), "RP_PSW op=0x%02X rp=%u", rp_psw_ops[o], rp);
      test_encode(buf, ((struct I8080Instr){.opcode = rp_psw_ops[o], .enc_t = ENC_REG_PAIR_PSW, .reg_1 = rp, .size = 1}), 1, rp_psw_ops[o] | (rp << 4), 0, 0);
    }
  }

  // ENC_REG_PAIR_SP_IMM16
  for (uint8_t rp = 0; rp < 4; rp++) {
    uint16_t val = 0x5678 + rp * 0x1111;
    snprintf(buf, sizeof(buf), "LXI rp=%u, 0x%04X", rp, val);
    test_encode(buf, ((struct I8080Instr){.opcode = 0x01, .enc_t = ENC_REG_PAIR_SP_IMM16, .reg_1 = rp, .num = val, .size = 3}),
                3, 0x01 + (rp << 4), val & 0xFF, (val >> 8) & 0xFF);
  }

  // ENC_REG_IMM8
  for (uint8_t reg = 0; reg < 8; reg++) {
    uint8_t imm = 0xA0 + reg;
    snprintf(buf, sizeof(buf), "MVI reg=%u, 0x%02X", reg, imm);
    test_encode(buf, ((struct I8080Instr){.opcode = 0x06, .enc_t = ENC_REG_IMM8, .reg_1 = reg, .num = imm, .size = 2}),
                2, 0x06 + (reg << 3), imm, 0);
  }

  // ENC_IMM8
  static const uint8_t imm8_ops[] = { 0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE, 0xD3, 0xDB };
  for (size_t i = 0; i < sizeof(imm8_ops); i++) {
    uint8_t imm = 0x42 + i;
    snprintf(buf, sizeof(buf), "IMM8 op=0x%02X, 0x%02X", imm8_ops[i], imm);
    test_encode(buf, ((struct I8080Instr){.opcode = imm8_ops[i], .enc_t = ENC_IMM8, .num = imm, .size = 2}),
                2, imm8_ops[i], imm, 0);
  }

  // ENC_ADDR16
  static const uint8_t addr16_ops[] = {
    0xC3, 0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA, // jumps
    0xCD, 0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC, // calls
    0x32, 0x3A, 0x22, 0x2A // STA, LDA, SHLD, LHLD
  };
  for (size_t i = 0; i < sizeof(addr16_ops); i++) {
    uint16_t addr = 0x8000 + i * 0x100;
    snprintf(buf, sizeof(buf), "ADDR16 op=0x%02X, 0x%04X", addr16_ops[i], addr);
    test_encode(buf, ((struct I8080Instr){.opcode = addr16_ops[i], .enc_t = ENC_ADDR16, .num = addr, .size = 3}),
                3, addr16_ops[i], addr & 0xFF, (addr >> 8) & 0xFF);
  }

  // ENC_RST
  for (uint8_t v = 0; v < 8; v++) {
    snprintf(buf, sizeof(buf), "RST %u", v);
    test_encode(buf, ((struct I8080Instr){.opcode = 0xC7, .enc_t = ENC_RST, .num = v, .size = 1}),
                1, 0xC7 | (v << 3), 0, 0);
  }

  // ENC_NONE
  test_encode("ENC_NONE", ((struct I8080Instr){.opcode = 0, .enc_t = ENC_NONE, .size = 0}), 0, 0, 0, 0);

  printf("Encode Results: %d / %d tests passed\n", passed_tests, total_tests);
  return (passed_tests == total_tests) ? 0 : 1;
}
