#include "cartridge_atari2600.hh"
#include <fstream>
#include <iostream>

bool CartridgeAtari2600::load(const std::string& path) {
    // Load the ROM file into the rom vector
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false; // Failed to open the file
    }

    // Read the entire file into the rom vector
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    this->rom.resize(size);
    file.read(reinterpret_cast<char*>(this->rom.data()), size);
    file.close();

    return true; // Successfully loaded the ROM
}

void CartridgeAtari2600::reset() {
    this->rom.clear(); // Clear the ROM data
}

uint8_t CartridgeAtari2600::read(uint16_t address) {
    if (address < this->rom.size()) {
        return this->rom[address]; // Return the byte at the specified address
    } else {
        return 0xFF; // Return 0xFF for out-of-bounds addresses
    }
}