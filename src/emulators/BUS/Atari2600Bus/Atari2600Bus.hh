#pragma once

#include <cstdint>
#include <string>
#include "CPU65.hh"
#include "Cartbridge.hh"
#include "MOS6532.hh"
#include "TIA1A.hh"
#include "Bus.hh"

class Atari2600Bus : public Bus {
public:
    Atari2600Bus();

    void loadROM(std::string &filename) override;
    void reset() override;

    uint8_t readMemory(uint16_t address) override;
    void writeMemory(uint16_t address, uint8_t value) override;

    void tick() override; // Advance the bus by one clock cycle
private:
    CPU65 cpu; // Instance of the CPU65 class for the Atari 2600
    Cartbridge cartbridge; // Instance of the Cartbridge class for ROM handling
    MOS6532 mos6532; // Instance of the MOS6532 class for RAM and I/O handling
    TIA1A tia1a; // Instance of the TIA1A class for video output
};