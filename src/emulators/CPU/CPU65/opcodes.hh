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
    
    // Add more opcodes as needed
};