#pragma once

/**
 * @file opcodes.hh
 * @brief Opcode byte values for the 6502 instruction set used by CPU65,
 *        including the documented instructions and the common
 *        undocumented/"illegal" opcodes.
 *
 * Values are grouped by mnemonic; the comment above each group names the
 * instruction and its addressing modes (e.g. IMM = immediate, ZP = zero
 * page, ZPX/ZPY = zero page indexed, ABS = absolute, ABSX/ABSY = absolute
 * indexed, INDX/INDY = indexed indirect / indirect indexed).
 */
enum Opcodes {
  // LDA (Load Accumulator) opcodes
  INS_LDA_IMM = 0xA9,
  INS_LDA_ZP = 0xA5,
  INS_LDA_ZPX = 0xB5,
  INS_LDA_ABS = 0xAD,
  INS_LDA_ABSX = 0xBD,
  INS_LDA_ABSY = 0xB9,
  INS_LDA_INDX = 0xA1,
  INS_LDA_INDY = 0xB1,

  // LDX (Load X Register) opcodes
  INS_LDX_IMM = 0xA2,
  INS_LDX_ZP = 0xA6,
  INS_LDX_ZPY = 0xB6,
  INS_LDX_ABS = 0xAE,
  INS_LDX_ABSY = 0xBE,

  // LDY (Load Y Register) opcodes
  INS_LDY_IMM = 0xA0,
  INS_LDY_ZP = 0xA4,
  INS_LDY_ZPX = 0xB4,
  INS_LDY_ABS = 0xAC,
  INS_LDY_ABSX = 0xBC,

  // ADC (Add with Carry) opcodes
  INS_ADC_IMM = 0x69,
  INS_ADC_ZP = 0x65,
  INS_ADC_ZPX = 0x75,
  INS_ADC_ABS = 0x6D,
  INS_ADC_ABSX = 0x7D,
  INS_ADC_ABSY = 0x79,
  INS_ADC_INDX = 0x61,
  INS_ADC_INDY = 0x71,

  // AND (Logical AND) opcodes
  INS_AND_IMM = 0x29,
  INS_AND_ZP = 0x25,
  INS_AND_ZPX = 0x35,
  INS_AND_ABS = 0x2D,
  INS_AND_ABSX = 0x3D,
  INS_AND_ABSY = 0x39,
  INS_AND_INDX = 0x21,
  INS_AND_INDY = 0x31,

  // ASL (Arithmetic Shift Left) opcodes
  INS_ASL_ACC = 0x0A,
  INS_ASL_ZP = 0x06,
  INS_ASL_ZPX = 0x16,
  INS_ASL_ABS = 0x0E,
  INS_ASL_ABSX = 0x1E,

  // BCC (Branch if Carry Clear) opcode
  INS_BCC = 0x90,

  // BCS (Branch if Carry Set) opcode
  INS_BCS = 0xB0,

  // BEQ (Branch if Equal) opcode
  INS_BEQ = 0xF0,

  // BIT (Bit Test) opcodes
  INS_BIT_ZP = 0x24,
  INS_BIT_ABS = 0x2C,

  // BMI (Branch if Minus) opcode
  INS_BMI = 0x30,

  // BNE (Branch if Not Equal) opcode
  INS_BNE = 0xD0,

  // BPL (Branch if Positive) opcode
  INS_BPL = 0x10,

  // BRK (Force Interrupt) opcode
  INS_BRK = 0x00,

  // BVC (Branch if Overflow Clear) opcode
  INS_BVC = 0x50,

  // BVS (Branch if Overflow Set) opcode
  INS_BVS = 0x70,

  // CLC (Clear Carry Flag) opcode
  INS_CLC = 0x18,

  // CLD (Clear Decimal Mode) opcode
  INS_CLD = 0xD8,

  // CLI (Clear Interrupt Disable) opcode
  INS_CLI = 0x58,

  // CLV (Clear Overflow Flag) opcode
  INS_CLV = 0xB8,

  // CMP (Compare) opcodes
  INS_CMP_IMM = 0xC9,
  INS_CMP_ZP = 0xC5,
  INS_CMP_ZPX = 0xD5,
  INS_CMP_ABS = 0xCD,
  INS_CMP_ABSX = 0xDD,
  INS_CMP_ABSY = 0xD9,
  INS_CMP_INDX = 0xC1,
  INS_CMP_INDY = 0xD1,

  // CPX (Compare X Register) opcodes
  INS_CPX_IMM = 0xE0,
  INS_CPX_ZP = 0xE4,
  INS_CPX_ABS = 0xEC,

  // CPY (Compare Y Register) opcodes
  INS_CPY_IMM = 0xC0,
  INS_CPY_ZP = 0xC4,
  INS_CPY_ABS = 0xCC,

  // DEC (Decrement Memory) opcodes
  INS_DEC_ZP = 0xC6,
  INS_DEC_ZPX = 0xD6,
  INS_DEC_ABS = 0xCE,
  INS_DEC_ABSX = 0xDE,

  // DEX (Decrement X Register) opcode
  INS_DEX = 0xCA,

  // DEY (Decrement Y Register) opcode
  INS_DEY = 0x88,

  // EOR (Exclusive OR) opcodes
  INS_EOR_IMM = 0x49,
  INS_EOR_ZP = 0x45,
  INS_EOR_ZPX = 0x55,
  INS_EOR_ABS = 0x4D,
  INS_EOR_ABSX = 0x5D,
  INS_EOR_ABSY = 0x59,
  INS_EOR_INDX = 0x41,
  INS_EOR_INDY = 0x51,

  // INC (Increment Memory) opcodes
  INS_INC_ZP = 0xE6,
  INS_INC_ZPX = 0xF6,
  INS_INC_ABS = 0xEE,
  INS_INC_ABSX = 0xFE,

  // INX (Increment X Register) opcode
  INS_INX = 0xE8,

  // INY (Increment Y Register) opcode
  INS_INY = 0xC8,

  // JMP (Jump) opcodes
  INS_JMP_ABS = 0x4C,
  INS_JMP_IND = 0x6C,

  // JSR (Jump to Subroutine) opcode
  INS_JSR = 0x20,

  // LSR (Logical Shift Right) opcodes
  INS_LSR_ACC = 0x4A,
  INS_LSR_ZP = 0x46,
  INS_LSR_ZPX = 0x56,
  INS_LSR_ABS = 0x4E,
  INS_LSR_ABSX = 0x5E,

  // NOP (No Operation) opcode
  INS_NOP = 0xEA,

  // ORA (Logical Inclusive OR) opcodes
  INS_ORA_IMM = 0x09,
  INS_ORA_ZP = 0x05,
  INS_ORA_ZPX = 0x15,
  INS_ORA_ABS = 0x0D,
  INS_ORA_ABSX = 0x1D,
  INS_ORA_ABSY = 0x19,
  INS_ORA_INDX = 0x01,
  INS_ORA_INDY = 0x11,

  // PHA (Push Accumulator) opcode
  INS_PHA = 0x48,

  // PHP (Push Processor Status) opcode
  INS_PHP = 0x08,

  // PLA (Pull Accumulator) opcode
  INS_PLA = 0x68,

  // PLP (Pull Processor Status) opcode
  INS_PLP = 0x28,

  // ROL (Rotate Left) opcodes
  INS_ROL_ACC = 0x2A,
  INS_ROL_ZP = 0x26,
  INS_ROL_ZPX = 0x36,
  INS_ROL_ABS = 0x2E,
  INS_ROL_ABSX = 0x3E,

  // ROR (Rotate Right) opcodes
  INS_ROR_ACC = 0x6A,
  INS_ROR_ZP = 0x66,
  INS_ROR_ZPX = 0x76,
  INS_ROR_ABS = 0x6E,
  INS_ROR_ABSX = 0x7E,

  // RTI (Return from Interrupt) opcode
  INS_RTI = 0x40,

  // RTS (Return from Subroutine) opcode
  INS_RTS = 0x60,

  // SBC (Subtract with Carry) opcodes
  INS_SBC_IMM = 0xE9,
  INS_SBC_ZP = 0xE5,
  INS_SBC_ZPX = 0xF5,
  INS_SBC_ABS = 0xED,
  INS_SBC_ABSX = 0xFD,
  INS_SBC_ABSY = 0xF9,
  INS_SBC_INDX = 0xE1,
  INS_SBC_INDY = 0xF1,

  // SEC (Set Carry Flag) opcode
  INS_SEC = 0x38,

  // SED (Set Decimal Flag) opcode
  INS_SED = 0xF8,

  // SEI (Set Interrupt Disable) opcode
  INS_SEI = 0x78,

  // STA (Store Accumulator) opcodes
  INS_STA_ZP = 0x85,
  INS_STA_ZPX = 0x95,
  INS_STA_ABS = 0x8D,
  INS_STA_ABSX = 0x9D,
  INS_STA_ABSY = 0x99,
  INS_STA_INDX = 0x81,
  INS_STA_INDY = 0x91,

  // STX (Store X Register) opcodes
  INS_STX_ZP = 0x86,
  INS_STX_ZPY = 0x96,
  INS_STX_ABS = 0x8E,

  // STY (Store Y Register) opcodes
  INS_STY_ZP = 0x84,
  INS_STY_ZPX = 0x94,
  INS_STY_ABS = 0x8C,

  // TAX (Transfer Accumulator to X) opcode
  INS_TAX = 0xAA,

  // TAY (Transfer Accumulator to Y) opcode
  INS_TAY = 0xA8,

  // TSX (Transfer Stack Pointer to X) opcode
  INS_TSX = 0xBA,

  // TXA (Transfer X to Accumulator) opcode
  INS_TXA = 0x8A,

  // TXS (Transfer X to Stack Pointer) opcode
  INS_TXS = 0x9A,

  // TYA (Transfer Y to Accumulator) opcode
  INS_TYA = 0x98,

  // Illegal opcodes (undocumented)

  // LAX (Load Accumulator and X Register) opcodes
  INS_LAX_ZP = 0xA7,
  INS_LAX_ZPY = 0xB7,
  INS_LAX_ABS = 0xAF,
  INS_LAX_ABSY = 0xBF,
  INS_LAX_INDX = 0xA3,
  INS_LAX_INDY = 0xB3,

  // SAX (Store Accumulator and X Register) opcodes
  INS_SAX_ZP = 0x87,
  INS_SAX_ZPY = 0x97,
  INS_SAX_ABS = 0x8F,

  // DCP (Decrement Memory and Compare) opcodes
  INS_DCP_ZP = 0xC7,
  INS_DCP_ZPX = 0xD7,
  INS_DCP_ABS = 0xCF,
  INS_DCP_ABSX = 0xDF,
  INS_DCP_ABSY = 0xDB,
  INS_DCP_INDX = 0xC3,
  INS_DCP_INDY = 0xD3,

  // ISB (Increment Memory and Subtract with Carry) opcodes
  INS_ISB_ZP = 0xE7,
  INS_ISB_ZPX = 0xF7,
  INS_ISB_ABS = 0xEF,
  INS_ISB_ABSX = 0xFF,
  INS_ISB_ABSY = 0xFB,
  INS_ISB_INDX = 0xE3,
  INS_ISB_INDY = 0xF3,

  // SLO (Shift Left and OR with Accumulator) opcodes
  INS_SLO_ZP = 0x07,
  INS_SLO_ZPX = 0x17,
  INS_SLO_ABS = 0x0F,
  INS_SLO_ABSX = 0x1F,
  INS_SLO_ABSY = 0x1B,
  INS_SLO_INDX = 0x03,
  INS_SLO_INDY = 0x13,

  // RLA (Rotate Left and AND with Accumulator) opcodes
  INS_RLA_ZP = 0x27,
  INS_RLA_ZPX = 0x37,
  INS_RLA_ABS = 0x2F,
  INS_RLA_ABSX = 0x3F,
  INS_RLA_ABSY = 0x3B,
  INS_RLA_INDX = 0x23,
  INS_RLA_INDY = 0x33,

  // SRE (Shift Right and EOR with Accumulator) opcodes
  INS_SRE_ZP = 0x47,
  INS_SRE_ZPX = 0x57,
  INS_SRE_ABS = 0x4F,
  INS_SRE_ABSX = 0x5F,
  INS_SRE_ABSY = 0x5B,
  INS_SRE_INDX = 0x43,
  INS_SRE_INDY = 0x53,

  // RRA (Rotate Right and ADC with Accumulator) opcodes
  INS_RRA_ZP = 0x67,
  INS_RRA_ZPX = 0x77,
  INS_RRA_ABS = 0x6F,
  INS_RRA_ABSX = 0x7F,
  INS_RRA_ABSY = 0x7B,
  INS_RRA_INDX = 0x63,
  INS_RRA_INDY = 0x73,

  // AHX (AND High X) opcodes
  INS_AHX_ABSY = 0x9F,
  INS_AHX_INDY = 0x93,

  // SHY (Store High Y) opcode
  INS_SHY_ABSX = 0x9C,

  // SHX (Store High X) opcode
  INS_SHX_ABSY = 0x9E,

  // TAS (Transfer Accumulator and Stack Pointer) opcode
  INS_TAS_ABSY = 0x9B,

  // XAA (Transfer Accumulator and X Register) opcode
  INS_XAA_IMM = 0x8B,

  // ANC (AND with Carry) opcode
  INS_ANC_IMM_0B = 0x0B,
  INS_ANC_IMM_2B = 0x2B,

  // ALR/ASR (AND with LSR) opcode
  INS_ALR_IMM = 0x4B,

  // ARR (AND with ROR) opcode
  INS_ARR_IMM = 0x6B,

  // AXS/SBX (AND with X and Subtract with Borrow) opcode
  INS_AXS_IMM = 0xCB,

  // LAS (Load Accumulator and Stack Pointer) opcode
  INS_LAS_ABSY = 0xBB,

  // KIL/JAM (Illegal Opcode) opcodes
  INS_KIL_02 = 0x02,
  INS_KIL_12 = 0x12,
  INS_KIL_22 = 0x22,
  INS_KIL_32 = 0x32,
  INS_KIL_42 = 0x42,
  INS_KIL_52 = 0x52,
  INS_KIL_62 = 0x62,
  INS_KIL_72 = 0x72,
  INS_KIL_92 = 0x92,
  INS_KIL_B2 = 0xB2,
  INS_KIL_D2 = 0xD2,
  INS_KIL_F2 = 0xF2,

  // SBC (Subtract with Carry) illegal opcode
  INS_SBC_IMM_ILLEGAL = 0xEB,

  // NOP (No Operation) illegal opcodes
  INS_NOP_1A = 0x1A,
  INS_NOP_3A = 0x3A,
  INS_NOP_5A = 0x5A,
  INS_NOP_7A = 0x7A,
  INS_NOP_DA = 0xDA,
  INS_NOP_FA = 0xFA,

  // NOP Immediate illegal opcodes
  INS_NOP_IMM_80 = 0x80,
  INS_NOP_IMM_82 = 0x82,
  INS_NOP_IMM_89 = 0x89,
  INS_NOP_IMM_C2 = 0xC2,
  INS_NOP_IMM_E2 = 0xE2,

  // NOP Zero Page illegal opcodes
  INS_NOP_ZP_04 = 0x04,
  INS_NOP_ZP_44 = 0x44,
  INS_NOP_ZP_64 = 0x64,

  // NOP Zero Page,X illegal opcodes
  INS_NOP_ZPX_14 = 0x14,
  INS_NOP_ZPX_34 = 0x34,
  INS_NOP_ZPX_54 = 0x54,
  INS_NOP_ZPX_74 = 0x74,
  INS_NOP_ZPX_D4 = 0xD4,
  INS_NOP_ZPX_F4 = 0xF4,

  // NOP Absolute illegal opcodes
  INS_NOP_ABS_0C = 0x0C,

  // NOP Absolute,X illegal opcodes
  INS_NOP_ABSX_1C = 0x1C,
  INS_NOP_ABSX_3C = 0x3C,
  INS_NOP_ABSX_5C = 0x5C,
  INS_NOP_ABSX_7C = 0x7C,
  INS_NOP_ABSX_DC = 0xDC,
  INS_NOP_ABSX_FC = 0xFC,
};
