#pragma once

#include <cstdint>
#include <string>
#include <vector>

class CPU65 {
public:
    CPU65(bool isCPU6502);

    void reset(bool isCPU6502);

    void execute(uint32_t cycles);

private:
    uint8_t A; // Accumulator
    uint8_t X; // Index Register X
    uint8_t Y; // Index Register Y
    
    uint16_t SP; // Stack Pointer
    uint16_t PC; // Program Counter
    
    uint8_t C : 1; // Carry Flag
    uint8_t Z : 1; // Zero Flag
    uint8_t I : 1; // Interrupt Disable Flag
    uint8_t D : 1; // Decimal Mode Flag
    uint8_t B : 1; // Break Command Flag
    uint8_t V : 1; // Overflow Flag
    uint8_t N : 1; // Negative Flag

    std::vector<uint8_t> memory; // Memory space for the CPU

    void reset();
    void zeroPageAddX(uint32_t &cycles, uint16_t &address);
    void zeroPageAddY(uint32_t &cycles, uint16_t &address);
    uint8_t fetch(uint32_t &cycles);
    uint16_t fetch16(uint32_t &cycles);
    uint8_t readMemory(uint32_t &cycles, uint16_t address);
    void writeMemory(uint32_t &cycles, uint16_t address, uint8_t value);

    void ldaSetFlags();
    void adcSetFlags(uint8_t value,uint16_t result);
    void andSetFlags();
    void aslSetFlags(uint32_t &cycles);
    void bitSetFlags(uint8_t value);
    void cmpSetFlags(uint8_t value);
};