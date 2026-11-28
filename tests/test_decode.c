#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include "i8080/i8080.h"

static int total_tests = 0;
static int passed_tests = 0;

static char buf[32];

static void test_decode(const char *name, uint8_t b0, uint8_t b1, uint8_t b2,
                        enum EncType exp_enc_t, uint8_t exp_sz,
                        int8_t exp_r1, int8_t exp_r2, long exp_num) {
  total_tests++;

  struct I8080Bytes instr = {
    .bytes = {b0, b1, b2},
    .size = exp_sz
  };

  struct I8080Instr out = i8080_decode_instr(instr);

  bool ok = (out.opcode == b0) &&
            (out.enc_t == exp_enc_t) &&
            (out.size == exp_sz) &&
            (exp_r1 < 0 || out.reg_1 == (uint8_t)exp_r1) &&
            (exp_r2 < 0 || out.reg_2 == (uint8_t)exp_r2) &&
            (exp_num < 0 || out.num == (uint16_t)exp_num);

  if (ok) {
    passed_tests++;
  } else {
    printf(
           "FAIL: %s (in: %02X %02X %02X -> "
            "enc=%d, sz=%u, r1=%d, r2=%d, num=0x%X; "
            "expected enc=%d, sz=%u, r1=%d, r2=%d, num=0x%lX)\n",
            name,
            b0, b1, b2,
            out.enc_t, out.size, out.reg_1, out.reg_2, out.num,
            exp_enc_t, exp_sz, exp_r1, exp_r2, exp_num
          );
  }
}

int main(void) {
  // ENC_FIXED
  test_decode("NOP",  0x00, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RLC",  0x07, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RRC",  0x0F, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RAL",  0x17, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RAR",  0x1F, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("DAA",  0x27, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("CMA",  0x2F, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("STC",  0x37, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("CMC",  0x3F, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("HLT",  0x76, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RET",  0xC9, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RNZ",  0xC0, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RZ",   0xC8, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RNC",  0xD0, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RC",   0xD8, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RPO",  0xE0, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RPE",  0xE8, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RP",   0xF0, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("RM",   0xF8, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("PCHL", 0xE9, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("SPHL", 0xF9, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("XTHL", 0xE3, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("XCHG", 0xEB, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("DI",   0xF3, 0, 0, ENC_FIXED, 1, -1, -1, -1);
  test_decode("EI",   0xFB, 0, 0, ENC_FIXED, 1, -1, -1, -1);

  // ENC_REG_REG
  for (uint8_t r1 = 0; r1 < 8; r1++) {
    for (uint8_t r2 = 0; r2 < 8; r2++) {
      if (r1 == 6 && r2 == 6) continue;
      uint8_t op = 0x40 | (r1 << 3) | r2;
      snprintf(buf, sizeof(buf), "MOV op=0x%02X", op);
      test_decode(buf, op, 0, 0, ENC_REG_REG, 1, r1, r2, -1);
    }
  }

  // ENC_REG_ID
  for (uint8_t reg = 0; reg < 8; reg++) {
    uint8_t inr_op = 0x04 | (reg << 3);
    uint8_t dcr_op = 0x05 | (reg << 3);
    snprintf(buf, sizeof(buf), "INR op=0x%02X", inr_op);
    test_decode(buf, inr_op, 0, 0, ENC_REG_ID, 1, reg, -1, -1);
    snprintf(buf, sizeof(buf), "DCR op=0x%02X", dcr_op);
    test_decode(buf, dcr_op, 0, 0, ENC_REG_ID, 1, reg, -1, -1);
  }

  // ENC_REG_ARITH
  static const uint8_t arith_bases[8] = { 0x80, 0x88, 0x90, 0x98, 0xA0, 0xA8, 0xB0, 0xB8 };
  for (int a = 0; a < 8; a++) {
    for (uint8_t reg = 0; reg < 8; reg++) {
      uint8_t op = arith_bases[a] | reg;
      snprintf(buf, sizeof(buf), "ARITH op=0x%02X", op);
      test_decode(buf, op, 0, 0, ENC_REG_ARITH, 1, reg, -1, -1);
    }
  }

  // ENC_REG_PAIR_SP
  static const uint8_t rp_sp_ops[] = { 0x03, 0x0B, 0x09 };
  for (int o = 0; o < 3; o++) {
    for (uint8_t rp = 0; rp < 4; rp++) {
      uint8_t op = rp_sp_ops[o] | (rp << 4);
      snprintf(buf, sizeof(buf), "RP_SP op=0x%02X", op);
      test_decode(buf, op, 0, 0, ENC_REG_PAIR_SP, 1, rp, -1, -1);
    }
  }

  // ENC_REG_PAIR_BD
  static const uint8_t rp_bd_ops[] = { 0x02, 0x0A };
  for (int o = 0; o < 2; o++) {
    for (uint8_t rp = 0; rp < 2; rp++) {
      uint8_t op = rp_bd_ops[o] | (rp << 4);
      snprintf(buf, sizeof(buf), "RP_BD op=0x%02X", op);
      test_decode(buf, op, 0, 0, ENC_REG_PAIR_BD, 1, rp, -1, -1);
    }
  }

  // ENC_REG_PAIR_PSW
  static const uint8_t rp_psw_ops[] = { 0xC5, 0xC1 };
  for (int o = 0; o < 2; o++) {
    for (uint8_t rp = 0; rp < 4; rp++) {
      uint8_t op = rp_psw_ops[o] | (rp << 4);
      snprintf(buf, sizeof(buf), "RP_PSW op=0x%02X", op);
      test_decode(buf, op, 0, 0, ENC_REG_PAIR_PSW, 1, rp, -1, -1);
    }
  }

  // ENC_REG_PAIR_SP_IMM16
  for (uint8_t rp = 0; rp < 4; rp++) {
    uint8_t op = 0x01 | (rp << 4);
    uint16_t num = 0xABCD;
    snprintf(buf, sizeof(buf), "LXI op=0x%02X", op);
    test_decode(buf, op, 0xCD, 0xAB, ENC_REG_PAIR_SP_IMM16, 3, rp, -1, num);
  }

  // ENC_REG_IMM8
  for (uint8_t reg = 0; reg < 8; reg++) {
    uint8_t op = 0x06 | (reg << 3);
    uint8_t imm = 0x77 + reg;
    snprintf(buf, sizeof(buf), "MVI op=0x%02X", op);
    test_decode(buf, op, imm, 0, ENC_REG_IMM8, 2, reg, -1, imm);
  }

  // ENC_IMM8
  static const uint8_t imm8_ops[] = { 0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE, 0xD3, 0xDB };
  for (size_t i = 0; i < sizeof(imm8_ops); i++) {
    uint8_t imm = 0x55 + i;
    snprintf(buf, sizeof(buf), "IMM8 op=0x%02X", imm8_ops[i]);
    test_decode(buf, imm8_ops[i], imm, 0, ENC_IMM8, 2, -1, -1, imm);
  }

  // ENC_ADDR16
  static const uint8_t addr16_ops[] = {
    0xC3, 0xC2, 0xCA, 0xD2, 0xDA, 0xE2, 0xEA, 0xF2, 0xFA,
    0xCD, 0xC4, 0xCC, 0xD4, 0xDC, 0xE4, 0xEC, 0xF4, 0xFC,
    0x32, 0x3A, 0x22, 0x2A
  };
  for (size_t i = 0; i < sizeof(addr16_ops); i++) {
    uint16_t addr = 0x1234 + i;
    snprintf(buf, sizeof(buf), "ADDR16 op=0x%02X", addr16_ops[i]);
    test_decode(buf, addr16_ops[i], addr & 0xFF, (addr >> 8) & 0xFF, ENC_ADDR16, 3, -1, -1, addr);
  }

  // ENC_RST
  for (uint8_t v = 0; v < 8; v++) {
    uint8_t op = 0xC7 | (v << 3);
    snprintf(buf, sizeof(buf), "RST op=0x%02X", op);
    test_decode(buf, op, 0, 0, ENC_RST, 1, -1, -1, v);
  }

  // Invalid opcodes (should return ENC_NONE)
  static const uint8_t invalid_opcodes[] = {
    0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
    0xCB, 0xD9, 0xDD, 0xED, 0xFD
  };
  for (size_t i = 0; i < sizeof(invalid_opcodes); i++) {
    snprintf(buf, sizeof(buf), "INVALID op=0x%02X", invalid_opcodes[i]);
    test_decode(buf, invalid_opcodes[i], 0, 0, ENC_NONE, 1, -1, -1, -1);
  }

  // Bytes -> Decode -> Encode for all 244 valid opcodes
  // Verify if the re-encoded bytes matches with the ones fed in.
  int re_encode_count = 0;
  for (int op = 0; op <= 0xFF; op++) {
    enum EncType enc = i8080_opcode_enc(op);
    if (enc == ENC_NONE) continue;

    // Bytes
    struct I8080Bytes bytes = { .bytes = {(uint8_t)op, 0x34, 0x12}, .size = 3 };

    // decode
    struct I8080Instr dec = i8080_decode_instr(bytes);

    // re-encode
    struct I8080Bytes enc_bytes = {0};
    i8080_encode_instr(dec, &enc_bytes);

    total_tests++;
    bool match = (enc_bytes.size == dec.size) && (enc_bytes.bytes[0] == (uint8_t)op);
    if (dec.size >= 2) match = match && (enc_bytes.bytes[1] == bytes.bytes[1]);
    if (dec.size >= 3) match = match && (enc_bytes.bytes[2] == bytes.bytes[2]);

    if (match) {
      passed_tests++;
      re_encode_count++;
    } else {
      printf("FAIL RE-ENCODING: op=0x%02X enc_t=%d size=%d (encoded %02X %02X %02X, expected %02X %02X %02X)\n",
             op, enc, dec.size, enc_bytes.bytes[0], enc_bytes.bytes[1], enc_bytes.bytes[2],
             bytes.bytes[0], bytes.bytes[1], bytes.bytes[2]);
    }
  }

  printf("Decode Results: %d / %d tests passed (including %d re-encoding)\n",
         passed_tests, total_tests, re_encode_count);
  return (passed_tests == total_tests) ? 0 : 1;
}
