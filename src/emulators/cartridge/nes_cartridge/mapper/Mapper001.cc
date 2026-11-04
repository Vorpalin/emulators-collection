#include "emulators/cartridge/nes_cartridge/mapper/Mapper001.hh"

uint8_t Mapper001::CpuRead(uint16_t addr) {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr < 0x8000) return 0;

  const size_t banks = PrgBanks16k();
  const size_t bank = prg_ & 0x0F;
  switch ((control_ >> 2) & 3) {
    case 0:
    case 1:  // 32 KiB mode: low bit of the bank number is ignored
      return ReadPrg(((bank & ~size_t{1}) % banks) * 0x4000 + (addr - 0x8000));
    case 2:  // first bank fixed at $8000, switchable at $C000
      if (addr < 0xC000) return ReadPrg(addr - 0x8000);
      return ReadPrg((bank % banks) * 0x4000 + (addr - 0xC000));
    default:  // switchable at $8000, last bank fixed at $C000
      if (addr < 0xC000) {
        return ReadPrg((bank % banks) * 0x4000 + (addr - 0x8000));
      }
      return ReadPrg((banks - 1) * 0x4000 + (addr - 0xC000));
  }
}

void Mapper001::CpuWrite(uint16_t addr, uint8_t value) {
  if (WritePrgRam(addr, value)) return;
  if (addr < 0x8000) return;

  if (value & 0x80) {  // reset
    shift_ = 0x10;
    control_ |= 0x0C;
    return;
  }
  const bool complete = shift_ & 1;
  shift_ = static_cast<uint8_t>((shift_ >> 1) | ((value & 1) << 4));
  if (complete) {
    switch ((addr >> 13) & 3) {
      case 0:
        control_ = shift_;
        break;
      case 1:
        chr0_ = shift_;
        break;
      case 2:
        chr1_ = shift_;
        break;
      default:
        prg_ = shift_;
        break;
    }
    shift_ = 0x10;
  }
}

uint8_t Mapper001::PpuRead(uint16_t addr) { return ReadChr(ChrOffset(addr)); }
void Mapper001::PpuWrite(uint16_t addr, uint8_t value) {
  WriteChr(ChrOffset(addr), value);
}

NesMirroring Mapper001::GetMirroring() const {
  switch (control_ & 3) {
    case 0:
      return NesMirroring::kSingleLow;
    case 1:
      return NesMirroring::kSingleHigh;
    case 2:
      return NesMirroring::kVertical;
    default:
      return NesMirroring::kHorizontal;
  }
}

void Mapper001::Reset() {
  shift_ = 0x10;
  control_ = 0x0C;
  chr0_ = chr1_ = prg_ = 0;
}

size_t Mapper001::ChrOffset(uint16_t addr) const {
  addr &= 0x1FFF;
  const size_t banks4k = rom_.chr.size() / 0x1000;
  if (control_ & 0x10) {  // two independent 4 KiB banks
    const size_t bank = (addr < 0x1000 ? chr0_ : chr1_) % banks4k;
    return bank * 0x1000 + (addr & 0x0FFF);
  }
  // 8 KiB mode: low bit of chr0 ignored
  return ((chr0_ & ~size_t{1}) % banks4k) * 0x1000 + addr;
}

void Mapper001::SaveState(nlohmann::json& j) const {
  Mapper::SaveState(j);
  j["shift"] = shift_;
  j["control"] = control_;
  j["chr0"] = chr0_;
  j["chr1"] = chr1_;
  j["prg"] = prg_;
}

void Mapper001::LoadState(const nlohmann::json& j) {
  Mapper::LoadState(j);
  shift_ = j.at("shift").get<uint8_t>();
  control_ = j.at("control").get<uint8_t>();
  chr0_ = j.at("chr0").get<uint8_t>();
  chr1_ = j.at("chr1").get<uint8_t>();
  prg_ = j.at("prg").get<uint8_t>();
}
