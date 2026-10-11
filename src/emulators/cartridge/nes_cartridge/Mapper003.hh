#pragma once

#include "emulators/cartridge/nes_cartridge/Mapper.hh"

namespace nes {

// CNROM: fixed PRG, switchable 8 KiB CHR bank.
class Mapper003 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override {
    uint8_t v = 0;
    if (ReadPrgRam(addr, &v)) return v;
    if (addr >= 0x8000) return ReadPrg(addr - 0x8000);
    return 0;
  }
  void CpuWrite(uint16_t addr, uint8_t value) override {
    if (WritePrgRam(addr, value)) return;
    if (addr >= 0x8000) chr_bank_ = value;
  }
  uint8_t PpuRead(uint16_t addr) override {
    return ReadChr((chr_bank_ % ChrBanks8k()) * 0x2000 + (addr & 0x1FFF));
  }
  void PpuWrite(uint16_t addr, uint8_t value) override {
    WriteChr((chr_bank_ % ChrBanks8k()) * 0x2000 + (addr & 0x1FFF), value);
  }
  void Reset() override { chr_bank_ = 0; }

 private:
  uint8_t chr_bank_ = 0;
};

}  // namespace nes
