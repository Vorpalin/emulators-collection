#include "emulators/cartridge/mirroring/nes_mirroring/NesMirroring.hh"

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
