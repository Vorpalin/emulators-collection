#pragma once

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

    //CLD (Clear Decimal Mode) opcode
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
    
    // Add more opcodes as needed
};