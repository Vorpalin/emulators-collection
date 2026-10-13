#include "emulators/cartridge/nes_cartridge/mapper/Mapper000.hh"

uint8_t Mapper000::CpuRead(uint16_t addr) override {
  uint8_t v = 0;
  if (ReadPrgRam(addr, &v)) return v;
  if (addr >= 0x8000) return ReadPrg(addr - 0x8000);
  return 0;
}

void Mapper000::CpuWrite(uint16_t addr, uint8_t value) override {
  WritePrgRam(addr, value);
}
uint8_t Mapper000::PpuRead(uint16_t addr) override {
  return ReadChr(addr & 0x1FFF);
}
void Mapper000::PpuWrite(uint16_t addr, uint8_t value) override {
  WriteChr(addr & 0x1FFF, value);
}
