#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

/**
 * @file SegaCartridge.hh
 * @brief Sega Master System / Game Gear cartridge with the standard Sega
 *        mapper and the Codemasters mapper.
 */

/** @brief Serializable cartridge state (everything except the ROM data). */
struct SegaCartridgeState {
  std::array<uint8_t, 3> bank{{0, 1, 2}};  ///< 16 KiB page of each slot.
  uint8_t ramControl = 0;                  ///< Sega mapper register 0xFFFC.
  std::vector<uint8_t> ram;                ///< 2 x 16 KiB cartridge RAM.
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SegaCartridgeState, bank, ramControl, ram)

/**
 * @class SegaCartridge
 * @brief ROM banking for the 0x0000-0xBFFF region of the Z80 address space.
 *
 * The address space is split in three 16 KiB slots, each showing one 16 KiB
 * ROM page.
 *
 * **Sega mapper** (almost every game). Registers live at the top of RAM and
 * are written together with it:
 *
 * | Address | Effect                                                        |
 * | ------- | ------------------------------------------------------------- |
 * | 0xFFFC  | bit 3: map cartridge RAM into slot 2, bit 2: RAM bank 0/1     |
 * | 0xFFFD  | page shown in slot 0 (the first 1 KiB always shows page 0)    |
 * | 0xFFFE  | page shown in slot 1                                          |
 * | 0xFFFF  | page shown in slot 2                                          |
 *
 * **Codemasters mapper** (detected from the ROM header checksum at 0x7FE0):
 * the page of slot 0, 1 and 2 is selected by writing to 0x0000, 0x4000 and
 * 0x8000. The extra RAM of "Cosmic Spacehead" is not emulated.
 *
 * ROMs with a 512-byte copier header are accepted; the header is dropped.
 * Page numbers wrap around the actual ROM size.
 *
 * The cartridge always exposes 32 KiB of RAM through the battery API, since
 * the ROM format does not say whether a game saves; a front-end only has to
 * persist it when consumeRamDirty() reports a change.
 */
class SegaCartridge {
 public:
  enum class Mapper { Sega, Codemasters };

  static constexpr std::size_t kPageSize = 0x4000;
  static constexpr std::size_t kRamSize = 0x8000;

  SegaCartridge();

  /**
   * @brief Loads a ROM image from memory and resets the mapper.
   * @return false (see error()) if the image is empty or too large (> 4 MiB).
   */
  bool load(const uint8_t *data, std::size_t size);
  bool loaded() const { return !rom_.empty(); }
  const std::string &error() const { return error_; }

  /** @brief Resets the mapper registers (cartridge RAM is kept). */
  void reset();

  /** @brief Reads from 0x0000-0xBFFF. */
  uint8_t read(uint16_t address) const;
  /** @brief Writes to 0x0000-0xBFFF (cartridge RAM, Codemasters registers). */
  void write(uint16_t address, uint8_t value);
  /** @brief Handles a write to 0xFFFC-0xFFFF (Sega mapper registers). */
  void writeMapper(uint16_t address, uint8_t value);

  Mapper mapper() const { return mapper_; }
  std::size_t pageCount() const { return pages_; }
  /** @brief Page currently shown in slot 0-2 (after wrapping). */
  std::size_t page(int slot) const { return state_.bank[static_cast<size_t>(slot)] % pages_; }

  /// @name Battery RAM
  /// @{
  bool hasBattery() const { return loaded(); }
  std::vector<uint8_t> batteryRam() const { return state_.ram; }
  /** @brief Restores RAM; accepts 16 KiB (bank 0 only) or 32 KiB images. */
  bool loadBatteryRam(const uint8_t *data, std::size_t size);
  /** @brief True if the game changed RAM since the last call. */
  bool consumeRamDirty() {
    bool d = dirty_;
    dirty_ = false;
    return d;
  }
  /// @}

  SegaCartridgeState getState() const { return state_; }
  void setState(const SegaCartridgeState &s);

 private:
  std::vector<uint8_t> rom_;  ///< Padded to a whole number of pages.
  std::size_t pages_ = 1;
  Mapper mapper_ = Mapper::Sega;
  SegaCartridgeState state_;
  std::string error_;
  bool dirty_ = false;

  bool ramMapped() const { return mapper_ == Mapper::Sega && (state_.ramControl & 0x08); }
  std::size_t ramOffset(uint16_t address) const {
    return ((state_.ramControl & 0x04) ? 0x4000u : 0u) + (address - 0x8000u);
  }
  bool detectCodemasters() const;
};
