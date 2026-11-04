#pragma once

#include <cstdint>

/**
 * @file NesMirroring.hh
 * @brief Nametable mirroring modes of the NES PPU.
 */

/** Mirroring modes for the PPU. */
enum class NesMirroring : uint8_t {
  kHorizontal,  ///< A A / B B  (vertical scrolling games)
  kVertical,    ///< A B / A B  (horizontal scrolling games)
  kSingleLow,   ///< Every nametable maps to the first 1 KiB page.
  kSingleHigh,  ///< Every nametable maps to the second 1 KiB page.
  kFourScreen,  ///< Four independent nametables (needs 4 KiB of VRAM).
};

/**
 * @brief Translate a PPU nametable address ($2000-$2FFF, mirrors included)
 *        into an offset in the console VRAM (2 KiB; 4 KiB for four-screen
 *        cartridges).
 * @details Must be called on every access: some mappers change the mirroring
 *          at runtime.
 * @param addr PPU address (only bits 0-11 are used).
 * @param mode Active mirroring mode.
 * @return Offset into the VRAM array.
 */
inline uint16_t MirrorNametableAddress(uint16_t addr, NesMirroring mode) {
  const uint16_t offset = addr & 0x03FF;
  const uint16_t table = (addr & 0x0FFF) >> 10;  // 0..3
  switch (mode) {
    case NesMirroring::kHorizontal:
      return static_cast<uint16_t>(((table >> 1) << 10) | offset);  // A A B B
    case NesMirroring::kVertical:
      return static_cast<uint16_t>(((table & 1) << 10) | offset);  // A B A B
    case NesMirroring::kSingleLow:
      return offset;
    case NesMirroring::kSingleHigh:
      return static_cast<uint16_t>(0x0400 | offset);
    case NesMirroring::kFourScreen:
      return static_cast<uint16_t>(addr & 0x0FFF);
  }
  return offset;
}
