#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "emulators/cartridge/nes_cartridge/Mirroring.hh"

namespace nes {

// Raw content of an iNES file, handed over to the mapper.
struct RomData {
  std::vector<uint8_t> prg_rom;
  std::vector<uint8_t> chr;  // CHR-ROM, or CHR-RAM when chr_is_ram
  bool chr_is_ram = false;
  Mirroring mirroring = Mirroring::kHorizontal;
  std::vector<uint8_t> prg_ram = std::vector<uint8_t>(0x2000, 0);
};

// What the cartridge exposes to the console. The CPU sees $4020-$FFFF,
// the PPU sees $0000-$1FFF (pattern tables). Nametables ($2000-$2FFF) are
// handled by the PPU through GetMirroring().
class Mapper {
 public:
  explicit Mapper(RomData rom) : rom_(std::move(rom)) {}
  virtual ~Mapper() = default;
  Mapper(const Mapper&) = delete;
  Mapper& operator=(const Mapper&) = delete;

  virtual uint8_t CpuRead(uint16_t addr) = 0;
  virtual void CpuWrite(uint16_t addr, uint8_t value) = 0;
  virtual uint8_t PpuRead(uint16_t addr) = 0;
  virtual void PpuWrite(uint16_t addr, uint8_t value) = 0;

  virtual Mirroring GetMirroring() const { return rom_.mirroring; }
  virtual void Reset() {}
  virtual void OnScanline() {}  // MMC3-style IRQ counter
  virtual bool IrqPending() const { return false; }

  std::vector<uint8_t>& PrgRam() { return rom_.prg_ram; }

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
  // PRG-RAM window $6000-$7FFF. Returns true if the address was handled.
  bool ReadPrgRam(uint16_t addr, uint8_t* out) const {
    if (addr < 0x6000 || addr > 0x7FFF || rom_.prg_ram.empty()) return false;
    *out = rom_.prg_ram[(addr - 0x6000) % rom_.prg_ram.size()];
    return true;
  }
  bool WritePrgRam(uint16_t addr, uint8_t value) {
    if (addr < 0x6000 || addr > 0x7FFF || rom_.prg_ram.empty()) return false;
    rom_.prg_ram[(addr - 0x6000) % rom_.prg_ram.size()] = value;
    return true;
  }
  size_t PrgBanks16k() const { return rom_.prg_rom.size() / 0x4000; }
  size_t ChrBanks8k() const { return rom_.chr.size() / 0x2000; }

  RomData rom_;
};

}  // namespace nes
