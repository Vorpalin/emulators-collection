#include "Atari2600Bus.hh"

#include <string>

Atari2600Bus::Atari2600Bus() : cpu(false), cartbridge(), mos6532(), tia1a() {
    // Initialize the Atari 2600 bus
}

void Atari2600Bus::loadROM(std::string &filename) {
    cartbridge.loadROM(filename);
}

void Atari2600Bus::reset() {
    cpu.reset(false); // Reset the CPU (false indicates it's not a 6502)
    cartbridge.reset(); // Reset the Cartbridge
    mos6532.reset(); // Reset the MOS6532
    tia1a.reset(); // Reset the TIA1A
}

void Atari2600Bus::tick() {
    // Advance the bus by one clock cycle
    uint32_t cycles = cpu.execute();
    mos6532.update(cycles); // Update the MOS6532 with the number of CPU cycles
    for (uint32_t i = 0; i < cycles * 3; ++i) {
        tia1a.tick(); // Tick the TIA1A for video output
    }
}