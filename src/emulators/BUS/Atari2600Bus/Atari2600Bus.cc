#include "Atari2600Bus.hh"

#include <string>

Atari2600Bus::Atari2600Bus() : cpu(false), cartbridge(), mos6532() {
    // Initialize the Atari 2600 bus
}

void Atari2600Bus::loadROM(std::string &filename) {
    cartbridge.loadROM(filename);
}

void Atari2600Bus::reset() {
    cpu.reset(false); // Reset the CPU (false indicates it's not a 6502)
    cartbridge.reset(); // Reset the Cartbridge
    mos6532.reset(); // Reset the MOS6532
}