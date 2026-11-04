#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

#include "emulators/cartridge/nes_cartridge/mapper/Mapper.hh"
#include "emulators/mirroring/nes_mirroring/NesMirroring.hh"

/**
 * @file NesCartridge.hh
 * @brief NES cartridge: iNES / NES 2.0 parsing and mapper dispatch.
 */

/** NES cartridge. Reads return 0 / writes are ignored while empty. */
class NesCartridge {
 public:
  NesCartridge() = default;

  /**
   * @brief Parse an iNES (or NES 2.0) image and instantiate its mapper.
   * @param data ROM bytes.
   * @param size Number of bytes.
   * @return false if the image is invalid or uses an unsupported mapper; the
   *         reason is then available through error().
   */
  bool load(const uint8_t* data, std::size_t size);

  bool loaded() const { return mapper_ != nullptr; }
  const std::string& error() const { return error_; }

  uint8_t cpuRead(uint16_t addr) {
    return mapper_ ? mapper_->CpuRead(addr) : 0;
  }
  void cpuWrite(uint16_t addr, uint8_t v) {
    if (mapper_) mapper_->CpuWrite(addr, v);
  }
  uint8_t ppuRead(uint16_t addr) {
    return mapper_ ? mapper_->PpuRead(addr) : 0;
  }
  void ppuWrite(uint16_t addr, uint8_t v) {
    if (mapper_) mapper_->PpuWrite(addr, v);
  }

  NesMirroring getMirroring() const {
    return mapper_ ? mapper_->GetMirroring() : NesMirroring::kHorizontal;
  }
  void reset() {
    if (mapper_) mapper_->Reset();
  }
  /// Notify the mapper of a filtered A12 rising edge (scanline counter).
  void onScanline() {
    if (mapper_) mapper_->OnScanline();
  }
  bool irqPending() const { return mapper_ && mapper_->IrqPending(); }

  int mapperId() const { return mapper_id_; }
  bool hasBattery() const { return has_battery_; }

  nlohmann::json saveState() const;
  void loadState(const nlohmann::json& j);

 private:
  std::unique_ptr<Mapper> mapper_;
  int mapper_id_ = 0;
  bool has_battery_ = false;
  std::string error_;
};
