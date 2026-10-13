#include "emulators/cartridge/nes_cartridge/mapper/Mapper002.hh"

uint8_t Mapper002::CpuRead(uint16_t addr) override {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr >= 0xC000) {
    return ReadPrg((PrgBanks16k() - 1) * 0x4000 + (addr - 0xC000));
  }
  if (addr >= 0x8000) {
    return ReadPrg((bank_ % PrgBanks16k()) * 0x4000 + (addr - 0x8000));
  }
  return 0;
}
void Mapper002::CpuWrite(uint16_t addr, uint8_t value) override {
  if (WritePrgRam(addr, value)) return;
  if (addr >= 0x8000) bank_ = value;
}
uint8_t Mapper002::PpuRead(uint16_t addr) override {
  return ReadChr(addr & 0x1FFF);
}
void Mapper002::PpuWrite(uint16_t addr, uint8_t value) override {
  WriteChr(addr & 0x1FFF, value);
}
void Mapper002::Reset() override { bank_ = 0; }
