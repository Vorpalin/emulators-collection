#include "Cartbridge.hh"

#include <fstream>
#include <iostream>

Cartbridge::Cartbridge() {
    reset();
}

void Cartbridge::reset() {
    romData.clear(); // Clear the ROM data vector
}

uint8_t Cartbridge::read(uint16_t address)
{
    if (romData.empty())
        return 0xFF;

    return romData[address % romData.size()];
}

void Cartbridge::loadROM(std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file)
    {
        std::cerr << "Could not open ROM: " << filename << '\n';
        return;
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    romData.resize(size);
    if (!file.read(reinterpret_cast<char*>(romData.data()), size))
    {
        std::cerr << "Error reading ROM: " << filename << '\n';
        romData.clear(); // Clear the ROM data vector on error
        return;
    }
}