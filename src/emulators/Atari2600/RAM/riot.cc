#include "riot.hh"

RIOT::RIOT() {
    this->reset();
}

void RIOT::reset() {
    for (auto& byte : ram) {
        byte = 0; // Initialize all RAM bytes to 0
    }
}

void RIOT::write(uint16_t address, uint8_t value) {
    if (address < ram.size()) {
        ram[address] = value; // Write the value to the specified address
    }
}

uint8_t RIOT::read(uint16_t address) {
    if (address < ram.size()) {
        return ram[address]; // Return the value at the specified address
    } else {
        return 0xFF; // Return 0xFF for out-of-bounds addresses
    }
}