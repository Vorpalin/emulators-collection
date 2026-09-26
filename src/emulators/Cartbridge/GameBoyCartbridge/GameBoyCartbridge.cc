#include "GameBoyCartbridge.hh"

#include <fstream>
#include <iostream>

GameBoyCartbridge::GameBoyCartbridge() { reset(); }

void GameBoyCartbridge::reset() {
  ramEnabled = false;
  romBankLow = 1;
  bankHigh = 0;
  mode1 = false;
}

void GameBoyCartbridge::detectMBC() {
  mbcType = MBCType::None;
  romBankCount = 2;
  ramBankCount = 0;

  if (romData.size() <= 0x0149) {
    std::cerr << "ROM too small to read header, assuming ROM only\n";
    return;
  }

  uint8_t cartType = romData[0x0147];
  uint8_t romSizeCode = romData[0x0148];
  uint8_t ramSizeCode = romData[0x0149];

  switch (cartType) {
    case 0x00:  // ROM ONLY
      mbcType = MBCType::None;
      break;
    case 0x01:  // MBC1
    case 0x02:  // MBC1+RAM
    case 0x03:  // MBC1+RAM+BATTERY
      mbcType = MBCType::MBC1;
      break;
    default:
      std::cerr << "Cartridge type 0x" << std::hex << static_cast<int>(cartType)
                << std::dec << " not supported, treating as MBC1\n";
      mbcType = MBCType::MBC1;
      break;
  }

  // ROM size: 32KB << romSizeCode, in 16KB (0x4000) banks.
  romBankCount = (romSizeCode <= 8) ? (2 << romSizeCode) : 2;

  // RAM size code -> number of 8KB (0x2000) banks.
  switch (ramSizeCode) {
    case 0x00:
      ramBankCount = 0;
      break;
    case 0x01:
      ramBankCount = 1;
      break;  // 2KB, treated as one partial bank
    case 0x02:
      ramBankCount = 1;
      break;  // 8KB
    case 0x03:
      ramBankCount = 4;
      break;  // 32KB
    case 0x04:
      ramBankCount = 16;
      break;  // 128KB
    case 0x05:
      ramBankCount = 8;
      break;  // 64KB
    default:
      ramBankCount = 0;
      break;
  }

  if (ramBankCount > 0) {
    ramData.assign(static_cast<size_t>(ramBankCount) * 0x2000, 0);
  } else {
    ramData.clear();
  }
}

int GameBoyCartbridge::currentRomBank() const {
  if (mbcType != MBCType::MBC1) return 1;

  int bank = romBankLow;
  if (!mode1)
    bank |= (bankHigh << 5);          // simple mode: bankHigh extends ROM bank
  if ((bank & 0x1F) == 0) bank |= 1;  // bank 0/0x20/0x40/0x60 read as +1
  return bank % romBankCount;
}

int GameBoyCartbridge::currentRamBank() const {
  if (mbcType != MBCType::MBC1) return 0;
  if (!mode1) return 0;  // simple mode: always RAM bank 0
  if (ramBankCount == 0) return 0;
  return bankHigh % ramBankCount;
}

uint8_t GameBoyCartbridge::read(uint16_t address) {
  if (romData.empty()) return 0xFF;

  if (address < 0x4000) {
    // Bank 0 is always fixed, even in MBC1 mode1 (only ROM bank switching
    // through the low register moves it, and that never touches bank 0).
    return romData[address];
  }

  if (address < 0x8000) {
    size_t offset =
        static_cast<size_t>(currentRomBank()) * 0x4000 + (address - 0x4000);
    if (offset >= romData.size()) return 0xFF;
    return romData[offset];
  }

  if (address >= 0xA000 && address < 0xC000) {
    if (!ramEnabled || ramData.empty()) return 0xFF;
    size_t offset =
        static_cast<size_t>(currentRamBank()) * 0x2000 + (address - 0xA000);
    if (offset >= ramData.size()) return 0xFF;
    return ramData[offset];
  }

  return 0xFF;
}

void GameBoyCartbridge::write(uint16_t address, uint8_t value) {
  if (mbcType == MBCType::None) {
    // ROM only carts can still have plain external RAM with no enable gate.
    if (address >= 0xA000 && address < 0xC000 && !ramData.empty()) {
      size_t offset = address - 0xA000;
      if (offset < ramData.size()) ramData[offset] = value;
    }
    return;
  }

  // MBC1 register writes.
  if (address < 0x2000) {
    ramEnabled = (value & 0x0F) == 0x0A;
  } else if (address < 0x4000) {
    romBankLow = value & 0x1F;
    if (romBankLow == 0) romBankLow = 1;
  } else if (address < 0x6000) {
    bankHigh = value & 0x03;
  } else if (address < 0x8000) {
    mode1 = value & 0x01;
  } else if (address >= 0xA000 && address < 0xC000) {
    if (!ramEnabled || ramData.empty()) return;
    size_t offset =
        static_cast<size_t>(currentRamBank()) * 0x2000 + (address - 0xA000);
    if (offset < ramData.size()) ramData[offset] = value;
  }
}

void GameBoyCartbridge::loadROM(std::string &filename) {
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
    romData.clear();
    return;
  }

  std::cout << "Loaded ROM: " << size << " bytes\n";
  detectMBC();
}
