#include "emulators/cartridge/atari2600_cartridge/Atari2600Cartridge.hh"

#include <fstream>
#include <iostream>

Atari2600Cartridge::Atari2600Cartridge() { reset(); }

void Atari2600Cartridge::reset() { bank = 1; }

uint8_t Atari2600Cartridge::read(uint16_t address) {
  if (romData.empty()) return 0xFF;

  if (romData.size() == 8192) return romData[bank * 4096 + (address % 4096)];

  return romData[address % romData.size()];
}

void Atari2600Cartridge::write(uint16_t address, uint8_t value) {
  (void)value;

  if (romData.size() != 8192) return;

  if (address == 0x0FF8)
    bank = 0;
  else if (address == 0x0FF9)
    bank = 1;
}

void Atari2600Cartridge::loadROM(std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file) {
    std::cerr << "Could not open ROM: " << filename << '\n';
    return;
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<uint8_t> buffer(static_cast<std::size_t>(size));
  if (!file.read(reinterpret_cast<char *>(buffer.data()), size)) {
    std::cerr << "Error reading ROM: " << filename << '\n';
    return;
  }
  loadROM(buffer.data(), buffer.size());
}

bool Atari2600Cartridge::loadROM(const uint8_t *data, std::size_t size) {
  if (data == nullptr || size == 0) {
    std::cerr << "Empty ROM\n";
    return false;
  }
  if (size > 8192) {
    std::cerr << "Unsupported ROM size: " << size << " bytes\n";
    return false;
  }
  romData.assign(data, data + size);
  return true;
}
