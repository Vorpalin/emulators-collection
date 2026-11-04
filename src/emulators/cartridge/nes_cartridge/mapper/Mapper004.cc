#include "emulators/cartridge/nes_cartridge/mapper/Mapper004.hh"

size_t Mapper004::PrgOffset(uint16_t addr) const {
  const size_t banks = PrgBanks8k();
  const size_t last = banks - 1;
  const bool swap = (bank_select_ & 0x40) != 0;
  size_t bank = 0;
  switch ((addr >> 13) & 3) {
    case 0:  // $8000-$9FFF
      bank = swap ? last - 1 : regs_[6];
      break;
    case 1:  // $A000-$BFFF
      bank = regs_[7];
      break;
    case 2:  // $C000-$DFFF
      bank = swap ? regs_[6] : last - 1;
      break;
    default:  // $E000-$FFFF
      bank = last;
      break;
  }
  return (bank % banks) * 0x2000 + (addr & 0x1FFF);
}

size_t Mapper004::ChrOffset(uint16_t addr) const {
  addr &= 0x1FFF;
  // With CHR inversion the two halves of the pattern space are swapped.
  const uint16_t a = (bank_select_ & 0x80) ? (addr ^ 0x1000) : addr;
  size_t bank = 0;
  if (a < 0x0800) {
    bank = (regs_[0] & 0xFE) | ((a >> 10) & 1);
  } else if (a < 0x1000) {
    bank = (regs_[1] & 0xFE) | ((a >> 10) & 1);
  } else {
    bank = regs_[2 + ((a - 0x1000) >> 10)];
  }
  return (bank % ChrBanks1k()) * 0x0400 + (addr & 0x03FF);
}

uint8_t Mapper004::CpuRead(uint16_t addr) {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr >= 0x8000) return ReadPrg(PrgOffset(addr));
  return 0;
}

void Mapper004::CpuWrite(uint16_t addr, uint8_t value) {
  if (WritePrgRam(addr, value)) return;
  if (addr < 0x8000) return;

  const bool even = (addr & 1) == 0;
  switch ((addr >> 13) & 3) {
    case 0:  // $8000-$9FFF
      if (even) {
        bank_select_ = value;
      } else {
        regs_[bank_select_ & 7] = value;
      }
      break;
    case 1:  // $A000-$BFFF
      if (even) horizontal_ = (value & 1) != 0;
      // Odd: PRG-RAM protect, deliberately ignored (always writable).
      break;
    case 2:  // $C000-$DFFF
      if (even) {
        irq_latch_ = value;
      } else {
        irq_counter_ = 0;
        irq_reload_ = true;
      }
      break;
    default:  // $E000-$FFFF
      if (even) {
        irq_enabled_ = false;
        irq_pending_ = false;
      } else {
        irq_enabled_ = true;
      }
      break;
  }
}

uint8_t Mapper004::PpuRead(uint16_t addr) { return ReadChr(ChrOffset(addr)); }

void Mapper004::PpuWrite(uint16_t addr, uint8_t value) {
  WriteChr(ChrOffset(addr), value);
}

NesMirroring Mapper004::GetMirroring() const {
  if (rom_.mirroring == NesMirroring::kFourScreen) return rom_.mirroring;
  return horizontal_ ? NesMirroring::kHorizontal : NesMirroring::kVertical;
}

void Mapper004::OnScanline() {
  if (irq_counter_ == 0 || irq_reload_) {
    irq_counter_ = irq_latch_;
  } else {
    --irq_counter_;
  }
  irq_reload_ = false;
  if (irq_counter_ == 0 && irq_enabled_) irq_pending_ = true;
}

void Mapper004::Reset() {
  bank_select_ = 0;
  regs_ = {0, 2, 4, 5, 6, 7, 0, 1};
  horizontal_ = rom_.mirroring == NesMirroring::kHorizontal;
  irq_latch_ = irq_counter_ = 0;
  irq_reload_ = irq_enabled_ = irq_pending_ = false;
}

void Mapper004::SaveState(nlohmann::json& j) const {
  Mapper::SaveState(j);
  j["bank_select"] = bank_select_;
  j["regs"] = regs_;
  j["horizontal"] = horizontal_;
  j["irq_latch"] = irq_latch_;
  j["irq_counter"] = irq_counter_;
  j["irq_reload"] = irq_reload_;
  j["irq_enabled"] = irq_enabled_;
  j["irq_pending"] = irq_pending_;
}

void Mapper004::LoadState(const nlohmann::json& j) {
  Mapper::LoadState(j);
  bank_select_ = j.at("bank_select").get<uint8_t>();
  regs_ = j.at("regs").get<std::array<uint8_t, 8>>();
  horizontal_ = j.at("horizontal").get<bool>();
  irq_latch_ = j.at("irq_latch").get<uint8_t>();
  irq_counter_ = j.at("irq_counter").get<uint8_t>();
  irq_reload_ = j.at("irq_reload").get<bool>();
  irq_enabled_ = j.at("irq_enabled").get<bool>();
  irq_pending_ = j.at("irq_pending").get<bool>();
}
