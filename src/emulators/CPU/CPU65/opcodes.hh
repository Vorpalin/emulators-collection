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
    
    // Add more opcodes as needed
};