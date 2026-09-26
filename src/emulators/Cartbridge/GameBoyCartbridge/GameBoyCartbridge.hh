#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @file GameBoyCartbridge.hh
 * @brief Game Boy cartridge (ROM + external RAM) emulation, including
 *        MBC1 bank switching. Reads the cartridge header (0x0147-0x0149)
 *        to figure out the cartridge type, ROM size and RAM size.
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

 private:
  enum class MBCType { None, MBC1 };

  void detectMBC();

  std::vector<uint8_t> romData;
  std::vector<uint8_t> ramData;

  MBCType mbcType = MBCType::None;
  int romBankCount = 2;  // number of 0x4000 ROM banks, min 2 (bank 0 + 1)
  int ramBankCount = 0;  // number of 0x2000 external RAM banks

  // MBC1 state
  bool ramEnabled = false;
  uint8_t romBankLow = 1;  // 5-bit register, bits 0-4 of the ROM bank
  uint8_t bankHigh = 0;    // 2-bit register: ROM bank bits 5-6, or RAM bank
  bool mode1 =
      false;  // false = simple (ROM banking) mode, true = RAM banking mode

  int currentRomBank() const;
  int currentRamBank() const;
};
