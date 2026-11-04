#pragma once

#include <array>

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"

/**
 * MMC3 (TxROM): 8 KiB PRG banks, 1 KiB / 2 KiB CHR banks, software-controlled
 * mirroring and a scanline counter that raises IRQs.
 */
class Mapper004 : public Mapper {
 public:
  using Mapper::Mapper;

  uint8_t CpuRead(uint16_t addr) override;
  void CpuWrite(uint16_t addr, uint8_t value) override;
  uint8_t PpuRead(uint16_t addr) override;
  void PpuWrite(uint16_t addr, uint8_t value) override;

  NesMirroring GetMirroring() const override;
  void Reset() override;
  void OnScanline() override;
  bool IrqPending() const override { return irq_pending_; }

  void SaveState(nlohmann::json& j) const override;
  void LoadState(const nlohmann::json& j) override;

 private:
  size_t PrgOffset(uint16_t addr) const;
  size_t ChrOffset(uint16_t addr) const;

  uint8_t bank_select_ = 0;
  std::array<uint8_t, 8> regs_{};  ///< R0-R7.
  bool horizontal_ = false;        ///< Mirroring bit written at $A000.

  uint8_t irq_latch_ = 0;
  uint8_t irq_counter_ = 0;
  bool irq_reload_ = false;
  bool irq_enabled_ = false;
  bool irq_pending_ = false;
};
