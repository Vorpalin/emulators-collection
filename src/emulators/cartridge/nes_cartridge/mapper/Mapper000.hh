#pragma once

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"

/**
 * NROM: no banking. 16 KiB PRG is mirrored in $8000-$FFFF.
 * CHR is fixed at $0000-$1FFF.
 */
class Mapper000 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override;
  void CpuWrite(uint16_t addr, uint8_t value) override;
  uint8_t PpuRead(uint16_t addr) override;
  void PpuWrite(uint16_t addr, uint8_t value) override;
};
