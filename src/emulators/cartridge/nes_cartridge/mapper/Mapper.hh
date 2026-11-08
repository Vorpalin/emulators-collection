#pragma once

#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <utility>
#include <vector>

#include "emulators/mirroring/nes_mirroring/NesMirroring.hh"

/**
 * @file Mapper.hh
 * @brief Base class of the NES cartridge mappers (bank switching hardware).
 */

/**
 * Raw content of an iNES file, handed over to the mapper.
 */
struct RomData {
  std::vector<uint8_t> prg_rom;
  std::vector<uint8_t> chr;
  bool chr_is_ram = false;
  NesMirroring mirroring = NesMirroring::kHorizontal;
  std::vector<uint8_t> prg_ram = std::vector<uint8_t>(0x2000, 0);
};

/**
 * Base class for all mappers.
 */
class Mapper {
 public:
  explicit Mapper(RomData rom) : rom_(std::move(rom)) {}
  virtual ~Mapper() = default;
  Mapper(const Mapper&) = delete;
  Mapper& operator=(const Mapper&) = delete;

  /// CPU read in $4020-$FFFF.
  virtual uint8_t CpuRead(uint16_t addr) = 0;
  /// CPU write in $4020-$FFFF.
  virtual void CpuWrite(uint16_t addr, uint8_t value) = 0;
  /// PPU read in $0000-$1FFF (pattern tables).
  virtual uint8_t PpuRead(uint16_t addr) = 0;
  /// PPU write in $0000-$1FFF (only effective with CHR-RAM).
  virtual void PpuWrite(uint16_t addr, uint8_t value) = 0;

  virtual NesMirroring GetMirroring() const { return rom_.mirroring; }
  virtual void Reset() {}

  /**
   * Called by the PPU on every (filtered) rising edge of the PPU address line
   * A12. Scanline counters such as the MMC3 one are clocked from it.
   */
  virtual void OnScanline() {}

  /// True while the mapper asserts the CPU IRQ line.
  virtual bool IrqPending() const { return false; }

  /// Serialize the mutable state (PRG-RAM, CHR-RAM and bank registers).
  virtual void SaveState(nlohmann::json& j) const {
    j["prg_ram"] = rom_.prg_ram;
    if (rom_.chr_is_ram) j["chr_ram"] = rom_.chr;
  }

  /// Restore a state produced by SaveState().
  virtual void LoadState(const nlohmann::json& j) {
    const auto ram = j.at("prg_ram").get<std::vector<uint8_t>>();
    if (ram.size() == rom_.prg_ram.size()) rom_.prg_ram = ram;
    if (rom_.chr_is_ram) {
      const auto chr = j.at("chr_ram").get<std::vector<uint8_t>>();
      if (chr.size() == rom_.chr.size()) rom_.chr = chr;
    }
  }

  std::vector<uint8_t>& PrgRam() { return rom_.prg_ram; }
  const std::vector<uint8_t>& PrgRam() const { return rom_.prg_ram; }

  /// Restores PRG-RAM (battery save). Size must match; not flagged as dirty.
  bool LoadPrgRam(const uint8_t* data, size_t size) {
    if (data == nullptr || size != rom_.prg_ram.size()) return false;
    rom_.prg_ram.assign(data, data + size);
    ram_dirty_ = false;
    return true;
  }

  /// True if the program changed PRG-RAM since the last call.
  bool ConsumeRamDirty() {
    const bool d = ram_dirty_;
    ram_dirty_ = false;
    return d;
  }

 protected:
  uint8_t ReadPrg(size_t offset) const {
    return rom_.prg_rom[offset % rom_.prg_rom.size()];
  }
  uint8_t ReadChr(size_t offset) const {
    return rom_.chr[offset % rom_.chr.size()];
  }
  void WriteChr(size_t offset, uint8_t value) {
    if (rom_.chr_is_ram) rom_.chr[offset % rom_.chr.size()] = value;
  }

  bool ReadPrgRam(uint16_t addr, uint8_t* out) const {
    if (addr < 0x6000 || addr > 0x7FFF || rom_.prg_ram.empty()) return false;
    *out = rom_.prg_ram[(addr - 0x6000) % rom_.prg_ram.size()];
    return true;
  }
  bool WritePrgRam(uint16_t addr, uint8_t value) {
    if (addr < 0x6000 || addr > 0x7FFF || rom_.prg_ram.empty()) return false;
    uint8_t& cell = rom_.prg_ram[(addr - 0x6000) % rom_.prg_ram.size()];
    if (cell != value) {
      cell = value;
      ram_dirty_ = true;
    }
    return true;
  }
  size_t PrgBanks16k() const { return rom_.prg_rom.size() / 0x4000; }
  size_t PrgBanks8k() const { return rom_.prg_rom.size() / 0x2000; }
  size_t PrgBanks32k() const {
    const size_t n = rom_.prg_rom.size() / 0x8000;
    return n == 0 ? 1 : n;
  }
  size_t ChrBanks8k() const {
    const size_t n = rom_.chr.size() / 0x2000;
    return n == 0 ? 1 : n;
  }
  size_t ChrBanks1k() const {
    const size_t n = rom_.chr.size() / 0x0400;
    return n == 0 ? 1 : n;
  }

  RomData rom_;

 private:
  bool ram_dirty_ = false;
};
