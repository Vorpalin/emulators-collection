#include "emulators/cartridge/nes_cartridge/mapper/Mapper003.hh"

uint8_t Mapper003::CpuRead(uint16_t addr) {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr >= 0x8000) return ReadPrg(addr - 0x8000);
  return 0;
}
void Mapper003::CpuWrite(uint16_t addr, uint8_t value) {
  if (WritePrgRam(addr, value)) return;
  if (addr >= 0x8000) chr_bank_ = value;
}
uint8_t Mapper003::PpuRead(uint16_t addr) {
  return ReadChr((chr_bank_ % ChrBanks8k()) * 0x2000 + (addr & 0x1FFF));
}
void Mapper003::PpuWrite(uint16_t addr, uint8_t value) {
  WriteChr((chr_bank_ % ChrBanks8k()) * 0x2000 + (addr & 0x1FFF), value);
}
void Mapper003::Reset() { chr_bank_ = 0; }

void Mapper003::SaveState(nlohmann::json& j) const {
  Mapper::SaveState(j);
  j["chr_bank"] = chr_bank_;
}

void Mapper003::LoadState(const nlohmann::json& j) {
  Mapper::LoadState(j);
  chr_bank_ = j.at("chr_bank").get<uint8_t>();
}
