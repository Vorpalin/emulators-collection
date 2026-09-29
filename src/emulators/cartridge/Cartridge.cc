#include "emulators/cartridge/Cartridge.hh"

#include <fstream>
#include <iostream>

Cartridge::Cartridge() { reset(); }

void Cartridge::reset() { bank = 1; }

uint8_t Cartridge::read(uint16_t address) {
  if (romData.empty()) return 0xFF;

  if (romData.size() == 8192) return romData[bank * 4096 + (address % 4096)];

  return romData[address % romData.size()];
}

void Cartridge::write(uint16_t address, uint8_t value) {
  (void)value;

  if (romData.size() != 8192) return;

  if (address == 0x0FF8)
    bank = 0;
  else if (address == 0x0FF9)
    bank = 1;
}

void Cartridge::loadROM(std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file) {
    std::cerr << "Could not open ROM: " << filename << '\n';
    return;
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  romData.resize(size);
  if (!file.read(reinterpret_cast<char *>(romData.data()), size)) {
    std::cerr << "Error reading ROM: " << filename << '\n';
    romData.clear();  // Clear the ROM data vector on error
    return;
  }

  std::cout << "Loaded ROM: " << size << " bytes\n";
}
