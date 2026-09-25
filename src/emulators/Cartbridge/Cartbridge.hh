#pragma once

#include <cstdint>
#include <string>
#include <vector>

/**
 * @file Cartbridge.hh
 * @brief Atari 2600 cartridge (ROM) emulation, including bank switching.
 */

/**
 * @class Cartbridge
 * @brief Holds a loaded ROM image and services CPU reads/writes into the
 *        cartridge address space, including bank-switching writes.
 */
class Cartbridge {
 public:
  /**
   * @brief Construct an empty cartridge (no ROM loaded).
   */
  Cartbridge();

  /**
   * @brief Load a ROM image from disk into memory.
   * @param filename Path to the ROM file to load.
   */
  void loadROM(std::string &filename);

  /**
   * @brief Reset cartridge state (e.g. current bank) to its default.
   */
  void reset();

  /**
   * @brief Read a byte from the cartridge, honoring the current bank.
   * @param address Address within the cartridge address space.
   * @return The byte value at that address.
   */
  uint8_t read(uint16_t address);

  /**
   * @brief Write to the cartridge address space; used to trigger bank
   *        switching on cartridges that support it.
   * @param address Address within the cartridge address space.
   * @param value   Byte value written (may be ignored, depending on the
   *                bank-switching scheme).
   */
  void write(uint16_t address, uint8_t value);

 private:
  std::vector<uint8_t> romData;  ///< Raw ROM image data.
  uint8_t bank = 0;  ///< Currently selected bank, for bank-switched cartridges.
};
