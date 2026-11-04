#pragma once

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"

/**
 * GxROM: one register selecting a 32 KiB PRG bank (bits 4-5) and an 8 KiB
 * CHR bank (bits 0-1).
 */
class Mapper066 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override;
  void CpuWrite(uint16_t addr, uint8_t value) override;
  uint8_t PpuRead(uint16_t addr) override;
  void PpuWrite(uint16_t addr, uint8_t value) override;

  void Reset() override { reg_ = 0; }

  void SaveState(nlohmann::json& j) const override;
  void LoadState(const nlohmann::json& j) override;

 private:
  size_t ChrOffset(uint16_t addr) const;

  uint8_t reg_ = 0;
};
