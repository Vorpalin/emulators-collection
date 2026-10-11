#pragma once

#include <cstdint>

namespace nes {

enum class Mirroring : uint8_t {
  kHorizontal,
  kVertical,
  kSingleLow,
  kSingleHigh,
  kFourScreen,
};

// Translates a PPU nametable address ($2000-$2FFF, mirrors included) into an
// offset in the console VRAM (2 KiB; 4 KiB for four-screen cartridges).
// Must be called on every access: some mappers change mirroring at runtime.
inline uint16_t MirrorNametableAddress(uint16_t addr, Mirroring mode) {
  const uint16_t offset = addr & 0x03FF;
  const uint16_t table = (addr & 0x0FFF) >> 10;  // 0..3
  switch (mode) {
    case Mirroring::kHorizontal:
      return static_cast<uint16_t>(((table >> 1) << 10) | offset);  // A A B B
    case Mirroring::kVertical:
      return static_cast<uint16_t>(((table & 1) << 10) | offset);  // A B A B
    case Mirroring::kSingleLow:
      return offset;
    case Mirroring::kSingleHigh:
      return static_cast<uint16_t>(0x0400 | offset);
    case Mirroring::kFourScreen:
      return static_cast<uint16_t>(addr & 0x0FFF);
  }
  return offset;
}

}  // namespace nes
