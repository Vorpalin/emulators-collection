#pragma once

#include <cstdint>

/** Mirroring modes for the PPU. */
enum class NesMirroring : uint8_t {
  kHorizontal,
  kVertical,
  kSingleLow,
  kSingleHigh,
  kFourScreen,
};

/** Translates a PPU nametable address ($2000-$2FFF, mirrors included) into an
 * offset in the console VRAM (2 KiB; 4 KiB for four-screen cartridges).
 * Must be called on every access: some mappers change mirroring at runtime.
 */
inline uint16_t MirrorNametableAddress(uint16_t addr, NesMirroring mode);
