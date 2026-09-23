#pragma once

#include <cstdint>

class CPU {
public:
    virtual uint32_t execute() = 0;

    virtual void reset() = 0;

    virtual uint8_t readMemory(uint32_t &cycles, uint16_t address) = 0;
    virtual void writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) = 0;
};