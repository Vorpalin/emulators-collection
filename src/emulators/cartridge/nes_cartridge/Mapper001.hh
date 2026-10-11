#pragma once

#include "emulators/cartridge/nes_cartridge/Mapper.hh"

namespace nes {

// MMC1: 5-bit serial shift register written one bit at a time ($8000-$FFFF).
// The 5th write commits the value into the register selected by A14-A13.
class Mapper001 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override {
    uint8_t v = 0;
    if (ReadPrgRam(addr, &v)) return v;
    if (addr < 0x8000) return 0;

    const size_t banks = PrgBanks16k();
    const size_t bank = prg_ & 0x0F;
    switch ((control_ >> 2) & 3) {
      case 0:
      case 1:  // 32 KiB mode: low bit of the bank number is ignored
        return ReadPrg(((bank & ~size_t{1}) % banks) * 0x4000 +
                       (addr - 0x8000));
      case 2:  // first bank fixed at $8000, switchable at $C000
        if (addr < 0xC000) return ReadPrg(addr - 0x8000);
        return ReadPrg((bank % banks) * 0x4000 + (addr - 0xC000));
      default:  // switchable at $8000, last bank fixed at $C000
        if (addr < 0xC000) {
          return ReadPrg((bank % banks) * 0x4000 + (addr - 0x8000));
        }
        return ReadPrg((banks - 1) * 0x4000 + (addr - 0xC000));
    }
  }

  void CpuWrite(uint16_t addr, uint8_t value) override {
    if (WritePrgRam(addr, value)) return;
    if (addr < 0x8000) return;

    if (value & 0x80) {  // reset
      shift_ = 0x10;
      control_ |= 0x0C;
      return;
    }
    const bool complete = shift_ & 1;
    shift_ = static_cast<uint8_t>((shift_ >> 1) | ((value & 1) << 4));
    if (complete) {
      switch ((addr >> 13) & 3) {
        case 0:
          control_ = shift_;
          break;
        case 1:
          chr0_ = shift_;
          break;
        case 2:
          chr1_ = shift_;
          break;
        default:
          prg_ = shift_;
          break;
      }
      shift_ = 0x10;
    }
  }

  uint8_t PpuRead(uint16_t addr) override { return ReadChr(ChrOffset(addr)); }
  void PpuWrite(uint16_t addr, uint8_t value) override {
    WriteChr(ChrOffset(addr), value);
  }

  Mirroring GetMirroring() const override {
    switch (control_ & 3) {
      case 0:
        return Mirroring::kSingleLow;
      case 1:
        return Mirroring::kSingleHigh;
      case 2:
        return Mirroring::kVertical;
      default:
        return Mirroring::kHorizontal;
    }
  }

  void Reset() override {
    shift_ = 0x10;
    control_ = 0x0C;
    chr0_ = chr1_ = prg_ = 0;
  }

 private:
  size_t ChrOffset(uint16_t addr) const {
    addr &= 0x1FFF;
    const size_t banks4k = rom_.chr.size() / 0x1000;
    if (control_ & 0x10) {  // two independent 4 KiB banks
      const size_t bank = (addr < 0x1000 ? chr0_ : chr1_) % banks4k;
      return bank * 0x1000 + (addr & 0x0FFF);
    }
    // 8 KiB mode: low bit of chr0 ignored
    return ((chr0_ & ~size_t{1}) % banks4k) * 0x1000 + addr;
  }

  uint8_t shift_ = 0x10;
  uint8_t control_ = 0x0C;  // PRG mode 3 at power-on
  uint8_t chr0_ = 0;
  uint8_t chr1_ = 0;
  uint8_t prg_ = 0;
};

}  // namespace nes
