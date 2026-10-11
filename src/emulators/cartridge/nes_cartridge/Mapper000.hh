#pragma once

#include "emulators/cartridge/nes_cartridge/Mapper.hh"

namespace nes {

// NROM: no banking. 16 KiB PRG is mirrored in $8000-$FFFF.
class Mapper000 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override {
    uint8_t v = 0;
    if (ReadPrgRam(addr, &v)) return v;
    if (addr >= 0x8000) return ReadPrg(addr - 0x8000);
    return 0;
  }
  void CpuWrite(uint16_t addr, uint8_t value) override {
    WritePrgRam(addr, value);
  }
  uint8_t PpuRead(uint16_t addr) override { return ReadChr(addr & 0x1FFF); }
  void PpuWrite(uint16_t addr, uint8_t value) override {
    WriteChr(addr & 0x1FFF, value);
  }
};

}  // namespace nes
