#pragma once

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"

/**
 * MMC1: 5-bit serial shift register written one bit at a time ($8000-$FFFF).
 * The 5th write commits the value into the register selected by A14-A13.
 */
class Mapper001 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override;

  void CpuWrite(uint16_t addr, uint8_t value) override;

  uint8_t PpuRead(uint16_t addr) override;
  void PpuWrite(uint16_t addr, uint8_t value) override;

  NesMirroring GetMirroring() const override;

  void Reset() override;

 private:
  size_t ChrOffset(uint16_t addr) const;

  uint8_t shift_ = 0x10;
  uint8_t control_ = 0x0C;  // PRG mode 3 at power-on
  uint8_t chr0_ = 0;
  uint8_t chr1_ = 0;
  uint8_t prg_ = 0;
};
