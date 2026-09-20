#pragma once

#include <cstdint>
#include <string>
#include "CPU65.hh"
#include "Cartbridge.hh"

class Atari2600Bus {
public:
    Atari2600Bus();

    void loadROM(std::string &filename);
    void reset();
private:
    CPU65 cpu; // Instance of the CPU65 class for the Atari 2600
    Cartbridge cartbridge; // Instance of the Cartbridge class for ROM handling
    // Add private members and methods for the Atari 2600 bus implementation
};