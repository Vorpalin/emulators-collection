#pragma once

#include <cstdint>
#include <string>
#include "CPU65.hh"
#include "Cartbridge.hh"
#include "MOS6532.hh"

class Atari2600Bus {
public:
    Atari2600Bus();

    void loadROM(std::string &filename);
    void reset();
private:
    CPU65 cpu; // Instance of the CPU65 class for the Atari 2600
    Cartbridge cartbridge; // Instance of the Cartbridge class for ROM handling
    MOS6532 mos6532; // Instance of the MOS6532 class for RAM and I/O handling
    // Add private members and methods for the Atari 2600 bus implementation
};