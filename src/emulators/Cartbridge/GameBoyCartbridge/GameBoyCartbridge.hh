#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @file GameBoyCartbridge.hh
 * @brief Game Boy cartridge (ROM + external RAM) emulation, including
 *        MBC1, MBC2, MBC3 and MBC5 bank switching. Reads the cartridge
 *        header (0x0147-0x0149) to detect the cartridge type and the
 *        ROM/RAM sizes.
 */
class GameBoyCartbridge {
 public:
  GameBoyCartbridge();

  void loadROM(std::string &filename);
  void reset();

  // address is the raw CPU/bus address (0x0000-0x7FFF for ROM,
  // 0xA000-0xBFFF for external RAM); this class does its own offsetting.
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);

  // For debugging: which ROM bank is currently mapped at 0x4000-0x7FFF.
  int getCurrentRomBank() const { return currentRomBank(); }

 private:
  enum class MBCType { None, MBC1, MBC2, MBC3, MBC5 };

  void detectMBC();

  std::vector<uint8_t> romData;
  std::vector<uint8_t> ramData;

  MBCType mbcType = MBCType::None;
  int romBankCount = 2;  // number of 0x4000 ROM banks, min 2 (bank 0 + 1)
  int ramBankCount = 0;  // number of 0x2000 external RAM banks

  bool ramEnabled = false;

  // MBC1 state
  uint8_t mbc1RomBankLow = 1;  // 5-bit register, bits 0-4 of the ROM bank
  uint8_t mbc1BankHigh = 0;    // 2-bit register: ROM bank bits 5-6, or RAM bank
  bool mbc1Mode1 = false;  // false = ROM banking mode, true = RAM banking mode

  // MBC2 state (has 512x4-bit built-in RAM, no external RAM chip)
  uint8_t mbc2RomBank = 1;  // 4-bit register

  // MBC3 state
  uint8_t mbc3RomBank = 1;   // 7-bit register, full ROM bank number
  uint8_t mbc3RamBank = 0;   // 0x00-0x03 RAM bank, or 0x08-0x0C RTC register
  uint8_t rtcRegs[5] = {0};  // seconds, minutes, hours, day-low, day-high
  uint8_t rtcLatchedRegs[5] = {0};
  uint8_t rtcLatchState = 0xFF;  // tracks the 0x00 -> 0x01 latch write sequence

  // MBC5 state
  uint16_t mbc5RomBank = 1;  // 9-bit register (0-511)
  uint8_t mbc5RamBank = 0;   // 4-bit register

  int currentRomBank() const;
  int currentRamBank() const;
};
