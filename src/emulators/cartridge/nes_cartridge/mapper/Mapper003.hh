#pragma once

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"

/**
 * CNROM: fixed PRG, switchable 8 KiB CHR bank.
 * CHR is switchable at $0000-$1FFF.
 */
class Mapper003 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override;
  void CpuWrite(uint16_t addr, uint8_t value) override;
  uint8_t PpuRead(uint16_t addr) override;
  void PpuWrite(uint16_t addr, uint8_t value) override;
  void Reset() override;

 private:
  uint8_t chr_bank_ = 0;
};
