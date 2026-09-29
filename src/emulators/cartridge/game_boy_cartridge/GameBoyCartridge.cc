#include "emulators/cartridge/game_boy_cartridge/GameBoyCartridge.hh"

#include <fstream>
#include <iostream>

GameBoyCartridge::GameBoyCartridge() { reset(); }

void GameBoyCartridge::reset() {
  ramEnabled = false;

  mbc1RomBankLow = 1;
  mbc1BankHigh = 0;
  mbc1Mode1 = false;

  mbc2RomBank = 1;

  mbc3RomBank = 1;
  mbc3RamBank = 0;
  for (auto &r : rtcRegs) r = 0;
  for (auto &r : rtcLatchedRegs) r = 0;
  rtcLatchState = 0xFF;

  mbc5RomBank = 1;
  mbc5RamBank = 0;
}

void GameBoyCartridge::detectMBC() {
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
      std::cout << "Detected ROM ONLY cartridge\n";
      break;
    case 0x01:  // MBC1
    case 0x02:  // MBC1+RAM
    case 0x03:  // MBC1+RAM+BATTERY
      mbcType = MBCType::MBC1;
      std::cout << "Detected MBC1 cartridge\n";
      break;
    case 0x05:  // MBC2
    case 0x06:  // MBC2+BATTERY
      mbcType = MBCType::MBC2;
      std::cout << "Detected MBC2 cartridge\n";
      break;
    case 0x0F:  // MBC3+TIMER+BATTERY
    case 0x10:  // MBC3+TIMER+RAM+BATTERY
    case 0x11:  // MBC3
    case 0x12:  // MBC3+RAM
    case 0x13:  // MBC3+RAM+BATTERY
      std::cout << "Detected MBC3 cartridge\n";
      mbcType = MBCType::MBC3;
      break;
    case 0x19:  // MBC5
    case 0x1A:  // MBC5+RAM
    case 0x1B:  // MBC5+RAM+BATTERY
    case 0x1C:  // MBC5+RUMBLE
    case 0x1D:  // MBC5+RUMBLE+RAM
    case 0x1E:  // MBC5+RUMBLE+RAM+BATTERY
      mbcType = MBCType::MBC5;
      std::cout << "Detected MBC5 cartridge\n";
      break;
    default:
      std::cerr << "Cartridge type 0x" << std::hex << static_cast<int>(cartType)
                << std::dec << " not supported, treating as MBC5\n";
      mbcType = MBCType::MBC5;  // widest addressing, safest generic fallback
      break;
  }

  // ROM size: 32KB << romSizeCode, in 16KB (0x4000) banks.
  romBankCount = (romSizeCode <= 8) ? (2 << romSizeCode) : 2;

  if (mbcType == MBCType::MBC2) {
    // MBC2 has a fixed 512x4-bit built-in RAM, not derived from the header.
    ramBankCount = 1;
    ramData.assign(512, 0);
    return;
  }

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

int GameBoyCartridge::currentRomBank() const {
  switch (mbcType) {
    case MBCType::MBC1: {
      int bank = mbc1RomBankLow | (mbc1BankHigh << 5);
      if ((bank & 0x1F) == 0) bank |= 1;  // bank 0/0x20/0x40/0x60 read as +1
      return bank % romBankCount;
    }
    case MBCType::MBC2:
      return mbc2RomBank % romBankCount;
    case MBCType::MBC3:
      return mbc3RomBank % romBankCount;
    case MBCType::MBC5:
      return mbc5RomBank % romBankCount;
    default:
      return 1;
  }
}

int GameBoyCartridge::currentRamBank() const {
  switch (mbcType) {
    case MBCType::MBC1:
      if (!mbc1Mode1) return 0;  // simple mode: always RAM bank 0
      if (ramBankCount == 0) return 0;
      return mbc1BankHigh % ramBankCount;
    case MBCType::MBC3:
      if (mbc3RamBank > 0x03 || ramBankCount == 0)
        return 0;  // RTC register selected instead
      return mbc3RamBank % ramBankCount;
    case MBCType::MBC5:
      if (ramBankCount == 0) return 0;
      return mbc5RamBank % ramBankCount;
    default:
      return 0;
  }
}

uint8_t GameBoyCartridge::read(uint16_t address) {
  if (romData.empty()) return 0xFF;

  if (address < 0x4000) {
    // Bank 0 is always fixed at this range for every MBC.
    return romData[address];
  }

  if (address < 0x8000) {
    size_t offset =
        static_cast<size_t>(currentRomBank()) * 0x4000 + (address - 0x4000);
    if (offset >= romData.size()) return 0xFF;
    return romData[offset];
  }

  if (address >= 0xA000 && address < 0xC000) {
    if (!ramEnabled) return 0xFF;

    if (mbcType == MBCType::MBC2) {
      // Only the low 9 bits of the address are wired; upper nibble floats high.
      size_t offset = (address - 0xA000) & 0x1FF;
      if (offset >= ramData.size()) return 0xFF;
      return static_cast<uint8_t>(0xF0 | (ramData[offset] & 0x0F));
    }

    if (mbcType == MBCType::MBC3 && mbc3RamBank >= 0x08 &&
        mbc3RamBank <= 0x0C) {
      // RTC register selected instead of RAM bank.
      return rtcLatchedRegs[mbc3RamBank - 0x08];
    }

    if (ramData.empty()) return 0xFF;
    size_t offset =
        static_cast<size_t>(currentRamBank()) * 0x2000 + (address - 0xA000);
    if (offset >= ramData.size()) return 0xFF;
    return ramData[offset];
  }

  return 0xFF;
}

void GameBoyCartridge::write(uint16_t address, uint8_t value) {
  if (mbcType == MBCType::None) {
    if (address >= 0xA000 && address < 0xC000 && !ramData.empty()) {
      size_t offset = address - 0xA000;
      if (offset < ramData.size()) ramData[offset] = value;
    }
    return;
  }

  switch (mbcType) {
    case MBCType::MBC1:
      if (address < 0x2000) {
        ramEnabled = (value & 0x0F) == 0x0A;
      } else if (address < 0x4000) {
        mbc1RomBankLow = value & 0x1F;
        if (mbc1RomBankLow == 0) mbc1RomBankLow = 1;
      } else if (address < 0x6000) {
        mbc1BankHigh = value & 0x03;
      } else if (address < 0x8000) {
        mbc1Mode1 = value & 0x01;
      } else if (address >= 0xA000 && address < 0xC000) {
        if (!ramEnabled || ramData.empty()) return;
        size_t offset =
            static_cast<size_t>(currentRamBank()) * 0x2000 + (address - 0xA000);
        if (offset < ramData.size()) ramData[offset] = value;
      }
      break;

    case MBCType::MBC2:
      if (address < 0x4000) {
        if (address & 0x0100) {
          mbc2RomBank = value & 0x0F;
          if (mbc2RomBank == 0) mbc2RomBank = 1;
        } else {
          ramEnabled = (value & 0x0F) == 0x0A;
        }
      } else if (address >= 0xA000 && address < 0xC000) {
        if (!ramEnabled) return;
        size_t offset = (address - 0xA000) & 0x1FF;
        if (offset < ramData.size()) ramData[offset] = value & 0x0F;
      }
      break;

    case MBCType::MBC3:
      if (address < 0x2000) {
        ramEnabled = (value & 0x0F) == 0x0A;
      } else if (address < 0x4000) {
        mbc3RomBank = value & 0x7F;
        if (mbc3RomBank == 0) mbc3RomBank = 1;
      } else if (address < 0x6000) {
        mbc3RamBank = value;  // 0x00-0x03 RAM bank, or 0x08-0x0C RTC register
      } else if (address < 0x8000) {
        // Latch clock data: a 0x00 write followed by a 0x01 write copies
        // the live RTC registers into the latched snapshot the CPU reads.
        if (rtcLatchState == 0x00 && value == 0x01) {
          for (int i = 0; i < 5; i++) rtcLatchedRegs[i] = rtcRegs[i];
        }
        rtcLatchState = value;
      } else if (address >= 0xA000 && address < 0xC000) {
        if (!ramEnabled) return;
        if (mbc3RamBank >= 0x08 && mbc3RamBank <= 0x0C) {
          rtcRegs[mbc3RamBank - 0x08] = value;  // write to live RTC register
          return;
        }
        if (ramData.empty()) return;
        size_t offset =
            static_cast<size_t>(currentRamBank()) * 0x2000 + (address - 0xA000);
        if (offset < ramData.size()) ramData[offset] = value;
      }
      break;

    case MBCType::MBC5:
      if (address < 0x2000) {
        ramEnabled = (value & 0x0F) == 0x0A;
      } else if (address < 0x3000) {
        mbc5RomBank = (mbc5RomBank & 0x100) | value;  // low 8 bits
      } else if (address < 0x4000) {
        mbc5RomBank =
            (mbc5RomBank & 0x0FF) | (static_cast<uint16_t>(value & 0x01) << 8);
      } else if (address < 0x6000) {
        mbc5RamBank =
            value & 0x0F;  // bit 3 doubles as the rumble motor on +RUMBLE carts
      } else if (address >= 0xA000 && address < 0xC000) {
        if (!ramEnabled || ramData.empty()) return;
        size_t offset =
            static_cast<size_t>(currentRamBank()) * 0x2000 + (address - 0xA000);
        if (offset < ramData.size()) ramData[offset] = value;
      }
      break;

    default:
      break;
  }
}

void GameBoyCartridge::loadROM(std::string &filename) {
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
