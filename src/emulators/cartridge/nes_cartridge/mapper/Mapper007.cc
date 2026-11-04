#include "emulators/cartridge/nes_cartridge/mapper/Mapper007.hh"

uint8_t Mapper007::CpuRead(uint16_t addr) {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr >= 0x8000) {
    return ReadPrg((reg_ & 0x07) % PrgBanks32k() * 0x8000 + (addr - 0x8000));
  }
  return 0;
}

void Mapper007::CpuWrite(uint16_t addr, uint8_t value) {
  if (WritePrgRam(addr, value)) return;
  if (addr >= 0x8000) reg_ = value;
}

uint8_t Mapper007::PpuRead(uint16_t addr) { return ReadChr(addr & 0x1FFF); }

void Mapper007::PpuWrite(uint16_t addr, uint8_t value) {
  WriteChr(addr & 0x1FFF, value);
}

NesMirroring Mapper007::GetMirroring() const {
  return (reg_ & 0x10) ? NesMirroring::kSingleHigh : NesMirroring::kSingleLow;
}

void Mapper007::SaveState(nlohmann::json& j) const {
  Mapper::SaveState(j);
  j["reg"] = reg_;
}

void Mapper007::LoadState(const nlohmann::json& j) {
  Mapper::LoadState(j);
  reg_ = j.at("reg").get<uint8_t>();
}
