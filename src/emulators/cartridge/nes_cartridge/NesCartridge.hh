#pragma once

#include <cstdint>
#include <istream>
#include <memory>
#include <string>

#include "emulators/cartridge/nes_cartridge/Mapper.hh"
#include "emulators/cartridge/nes_cartridge/Mirroring.hh"

/** NES cartridge. */
class NesCartridge {
 public:
  explicit NesCartridge();

  uint8_t cpuRead(uint16_t addr) { return mapper_->CpuRead(addr); }
  void cpuWrite(uint16_t addr, uint8_t v) { mapper_->CpuWrite(addr, v); }
  uint8_t ppuRead(uint16_t addr) { return mapper_->PpuRead(addr); }
  void ppuWrite(uint16_t addr, uint8_t v) { mapper_->PpuWrite(addr, v); }

  Mirroring getMirroring() const { return mapper_->GetMirroring(); }
  void reset() { mapper_->Reset(); }
  void onScanline() { mapper_->OnScanline(); }
  bool irqPending() const { return mapper_->IrqPending(); }

  int mapperId() const { return mapper_id_; }
  bool hasBattery() const { return has_battery_; }

  // Battery-backed PRG-RAM persistence (no-op if the cartridge has no battery).
  void loadSave(const std::string& path);
  void writeSave(const std::string& path) const;

  void load(std::istream& in);

 private:
  std::unique_ptr<Mapper> mapper_;
  int mapper_id_ = 0;
  bool has_battery_ = false;
};
