#pragma once

#include "emulators/cartridge/nes_cartridge/Mapper.hh"

namespace nes {

// UxROM: 16 KiB switchable bank at $8000, last bank fixed at $C000.
class Mapper002 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override {
    uint8_t v = 0;
    if (ReadPrgRam(addr, &v)) return v;
    if (addr >= 0xC000) {
      return ReadPrg((PrgBanks16k() - 1) * 0x4000 + (addr - 0xC000));
    }
    if (addr >= 0x8000) {
      return ReadPrg((bank_ % PrgBanks16k()) * 0x4000 + (addr - 0x8000));
    }
    return 0;
  }
  void CpuWrite(uint16_t addr, uint8_t value) override {
    if (WritePrgRam(addr, value)) return;
    if (addr >= 0x8000) bank_ = value;
  }
  uint8_t PpuRead(uint16_t addr) override { return ReadChr(addr & 0x1FFF); }
  void PpuWrite(uint16_t addr, uint8_t value) override {
    WriteChr(addr & 0x1FFF, value);
  }
  void Reset() override { bank_ = 0; }

 private:
  uint8_t bank_ = 0;
};

}  // namespace nes
