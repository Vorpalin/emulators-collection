#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

/**
 * @file Atari2600Cartridge.hh
 * @brief Atari 2600 cartridge (ROM) emulation, including bank switching.
 */

struct Atari2600CartridgeState {
  std::vector<uint8_t> romData;
  uint8_t bank = 0;
};

void to_json(nlohmann::json& j, const Atari2600CartridgeState& state);
void from_json(const nlohmann::json& j, Atari2600CartridgeState& state);

/**
 * @class Atari2600Cartridge
 * @brief Holds a loaded ROM image and services CPU reads/writes into the
 *        cartridge address space, including bank-switching writes.
 */
class Atari2600Cartridge {
 public:
  /**
   * @brief Construct an empty cartridge (no ROM loaded).
   */
  Atari2600Cartridge();

  /**
   * @brief Load a ROM image from disk into memory.
   * @param filename Path to the ROM file to load.
   */
  void loadROM(std::string& filename);

  /**
   * @brief Load a ROM image from memory (no file system needed).
   * @param data Pointer to the ROM bytes.
   * @param size Number of bytes (2K, 4K or 8K images are supported).
   * @return false if the image is empty or larger than 8K (unsupported
   *         bank-switching scheme); the previous ROM is then kept.
   */
  bool loadROM(const uint8_t* data, std::size_t size);

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

  void setState(const Atari2600CartridgeState& state);
  Atari2600CartridgeState getState() const;

 private:
  std::vector<uint8_t> romData;  ///< Raw ROM image data.
  uint8_t bank = 0;  ///< Currently selected bank, for bank-switched cartridges.
};
