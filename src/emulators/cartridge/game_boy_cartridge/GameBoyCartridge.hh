#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

/**
 * @file GameBoyCartridge.hh
 * @brief Game Boy cartridge (ROM + external RAM) emulation, including
 *        MBC1, MBC2, MBC3 and MBC5 bank switching. Reads the cartridge
 *        header (0x0147-0x0149) to detect the cartridge type and the
 *        ROM/RAM sizes.
 */

/**
 * @struct GameBoyCartridgeState
 * @brief Serializable state of a Game Boy cartridge.
 * @details This structure holds the state of the cartridge for the purpose of
 *          saving and loading the emulator's state.
 */
struct GameBoyCartridgeState {
  std::vector<uint8_t> romData;
  std::vector<uint8_t> ramData;

  bool ramEnabled = false;

  uint8_t mbc1RomBankLow = 1;
  uint8_t mbc1BankHigh = 0;
  bool mbc1Mode1 = false;

  uint8_t mbc2RomBank = 1;

  uint8_t mbc3RomBank = 1;
  uint8_t mbc3RamBank = 0;
  uint8_t rtcRegs[5] = {0};
  uint8_t rtcLatchedRegs[5] = {0};
  uint8_t rtcLatchState = 0xFF;

  uint16_t mbc5RomBank = 1;
  uint8_t mbc5RamBank = 0;
};

/**
 * @brief Serializes a GameBoyCartridgeState to JSON.
 * @param j The JSON object to populate.
 * @param state The GameBoyCartridgeState to serialize.
 */
void to_json(nlohmann::json& j, const GameBoyCartridgeState& state);

/**
 * @brief Deserializes a GameBoyCartridgeState from JSON.
 * @param j The JSON object to read from.
 * @param state The GameBoyCartridgeState to populate.
 */
void from_json(const nlohmann::json& j, GameBoyCartridgeState& state);

/**
 * @class GameBoyCartridge
 * @brief Cartridge with memory bank controller (MBC) emulation.
 *
 * Holds the ROM image and external RAM, and translates CPU accesses in
 * 0x0000-0x7FFF (ROM) and 0xA000-0xBFFF (external RAM) into accesses to the
 * currently selected banks. Writes to the ROM area are interpreted as MBC
 * register writes (RAM enable, bank select, mode select, RTC latch...).
 */
class GameBoyCartridge {
 public:
  /** @brief Constructs an empty cartridge (no ROM loaded). */
  GameBoyCartridge();

  /**
   * @brief Loads a ROM file and configures the MBC from its header.
   * @param filename Path to the ROM file.
   */
  void loadROM(std::string& filename);

  /**
   * @brief Load a ROM image from memory (no file system needed).
   * @param data Pointer to the ROM bytes.
   * @param size Number of bytes (must at least cover the 0x150-byte header).
   * @return false if the image is too small to be a Game Boy ROM.
   */
  bool loadROM(const uint8_t* data, std::size_t size);

  /** @brief Resets all MBC registers and banks to their power-on values. */
  void reset();

  /**
   * @brief Reads a byte from the cartridge.
   * @param address Raw CPU/bus address (0x0000-0x7FFF for ROM,
   *        0xA000-0xBFFF for external RAM); this class does its own
   *        offsetting.
   * @return The byte read (0xFF for disabled/absent RAM).
   */
  uint8_t read(uint16_t address);

  /**
   * @brief Writes a byte to the cartridge.
   *
   * Addresses in 0x0000-0x7FFF drive the MBC registers; addresses in
   * 0xA000-0xBFFF write external RAM (or RTC registers on MBC3).
   *
   * @param address Raw CPU/bus address.
   * @param value   Value written.
   */
  void write(uint16_t address, uint8_t value);

  /**
   * @brief Returns the ROM bank currently mapped at 0x4000-0x7FFF.
   * @note Intended for debugging.
   */
  int getCurrentRomBank() const { return currentRomBank(); }

  /**
   * @brief Returns the external RAM bank currently mapped at 0xA000-0xBFFF.
   * @note Intended for debugging.
   */
  void setState(const GameBoyCartridgeState& state);

  /**
   * @brief Returns the current state of the cartridge for serialization.
   * @return A GameBoyCartridgeState object representing the current state.
   */
  GameBoyCartridgeState getState() const;

 private:
  /** @brief Supported memory bank controller types. */
  enum class MBCType { None, MBC1, MBC2, MBC3, MBC5 };

  /** @brief Detects MBC type, ROM size and RAM size from the header. */
  void detectMBC();

  std::vector<uint8_t> romData;  ///< Whole ROM image.
  std::vector<uint8_t> ramData;  ///< External (battery) RAM.

  MBCType mbcType = MBCType::None;  ///< Detected MBC type.
  int romBankCount = 2;  ///< Number of 0x4000 ROM banks, min 2 (bank 0 + 1).
  int ramBankCount = 0;  ///< Number of 0x2000 external RAM banks.

  bool ramEnabled =
      false;  ///< External RAM enable flag (0x0000-0x1FFF writes).

  /// @name MBC1 state
  /// @{
  uint8_t mbc1RomBankLow = 1;  ///< 5-bit register, bits 0-4 of the ROM bank.
  uint8_t mbc1BankHigh =
      0;  ///< 2-bit register: ROM bank bits 5-6, or RAM bank.
  bool mbc1Mode1 =
      false;  ///< false = ROM banking mode, true = RAM banking mode.
  /// @}

  /// @name MBC2 state
  /// MBC2 has 512x4-bit built-in RAM and no external RAM chip.
  /// @{
  uint8_t mbc2RomBank = 1;  ///< 4-bit ROM bank register.
  /// @}

  /// @name MBC3 state
  /// @{
  uint8_t mbc3RomBank = 1;  ///< 7-bit register, full ROM bank number.
  uint8_t mbc3RamBank = 0;  ///< 0x00-0x03 RAM bank, or 0x08-0x0C RTC register.
  uint8_t rtcRegs[5] = {
      0};  ///< RTC: seconds, minutes, hours, day-low, day-high.
  uint8_t rtcLatchedRegs[5] = {
      0};  ///< Latched copy of #rtcRegs, read by the CPU.
  uint8_t rtcLatchState =
      0xFF;  ///< Tracks the 0x00 -> 0x01 latch write sequence.
  /// @}

  /// @name MBC5 state
  /// @{
  uint16_t mbc5RomBank = 1;  ///< 9-bit ROM bank register (0-511).
  uint8_t mbc5RamBank = 0;   ///< 4-bit RAM bank register.
  /// @}

  /** @brief Computes the ROM bank mapped at 0x4000-0x7FFF for the active MBC.
   */
  int currentRomBank() const;
  /** @brief Computes the external RAM bank currently mapped at 0xA000-0xBFFF.
   */
  int currentRamBank() const;
};
