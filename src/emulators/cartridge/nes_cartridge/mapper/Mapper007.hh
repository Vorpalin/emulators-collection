#pragma once

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"

/**
 * AxROM: 32 KiB switchable PRG bank and one-screen mirroring selected by
 * bit 4 of the written value. CHR is RAM.
 */
class Mapper007 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override;
  void CpuWrite(uint16_t addr, uint8_t value) override;
  uint8_t PpuRead(uint16_t addr) override;
  void PpuWrite(uint16_t addr, uint8_t value) override;

  NesMirroring GetMirroring() const override;
  void Reset() override { reg_ = 0; }

  void SaveState(nlohmann::json& j) const override;
  void LoadState(const nlohmann::json& j) override;

 private:
  uint8_t reg_ = 0;
};
