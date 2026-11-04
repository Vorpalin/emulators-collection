#include "emulators/cartridge/nes_cartridge/mapper/Mapper066.hh"

uint8_t Mapper066::CpuRead(uint16_t addr) {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr >= 0x8000) {
    const size_t bank = ((reg_ >> 4) & 3) % PrgBanks32k();
    return ReadPrg(bank * 0x8000 + (addr - 0x8000));
  }
  return 0;
}

void Mapper066::CpuWrite(uint16_t addr, uint8_t value) {
  if (WritePrgRam(addr, value)) return;
  if (addr >= 0x8000) reg_ = value;
}

size_t Mapper066::ChrOffset(uint16_t addr) const {
  return ((reg_ & 3) % ChrBanks8k()) * 0x2000 + (addr & 0x1FFF);
}

uint8_t Mapper066::PpuRead(uint16_t addr) { return ReadChr(ChrOffset(addr)); }

void Mapper066::PpuWrite(uint16_t addr, uint8_t value) {
  WriteChr(ChrOffset(addr), value);
}

void Mapper066::SaveState(nlohmann::json& j) const {
  Mapper::SaveState(j);
  j["reg"] = reg_;
}

void Mapper066::LoadState(const nlohmann::json& j) {
  Mapper::LoadState(j);
  reg_ = j.at("reg").get<uint8_t>();
}
