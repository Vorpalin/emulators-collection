#include "Atari2600Bus.hh"

#include <string>

Atari2600Bus::Atari2600Bus() : cpu(this), cartbridge(), mos6532(), tia1a() {
  // Initialize the Atari 2600 bus
}

void Atari2600Bus::loadROM(std::string &filename) {
  cartbridge.loadROM(filename);
}

uint8_t Atari2600Bus::readMemory(uint16_t address) {
  address &= 0x1FFF;

  if (address & 0x1000)  // A12 set -> cartridge
    return cartbridge.read(address & 0x0FFF);
  if (!(address & 0x0080))  // A7 clear -> TIA
    return tia1a.read(address & 0x3F);
  if (!(address &
        0x0200))  // A9 clear -> RIOT RAM (0x80-0xFF, mirrored at 0x180-0x1FF)
    return mos6532.read(address & 0x7F);
  return mos6532.read(0x80 |
                      (address & 0x1F));  // RIOT I/O + timer (0x280-0x29F)
}

void Atari2600Bus::writeMemory(uint16_t address, uint8_t value) {
  address &= 0x1FFF;

  if (address & 0x1000)
    cartbridge.write(address & 0x0FFF, value);
  else if (!(address & 0x0080)) {
    const uint16_t reg = address & 0x3F;
    tia1a.write(reg, value);
    if (onAudioWrite && reg >= 0x15 && reg <= 0x1A) onAudioWrite(reg, value);
  } else if (!(address & 0x0200))
    mos6532.write(address & 0x7F, value);
  else
    mos6532.write(0x80 | (address & 0x1F), value);
}

void Atari2600Bus::reset() {
  cartbridge.reset();  // Reset the Cartbridge
  cpu.reset();         // Reset the CPU
  mos6532.reset();     // Reset the MOS6532
  tia1a.reset();       // Reset the TIA1A
}

void Atari2600Bus::tick() {
  if (tia1a.cpuHalted()) {
    int n = 0;
    do {
      tia1a.tick();
      if (++n == 3) {
        n = 0;
        mos6532.update(1);
      }
    } while (tia1a.cpuHalted());
    return;
  }

  // Advance the bus by one clock cycle
  uint32_t cpuCycles = cpu.execute();

  for (uint32_t i = 0; i < cpuCycles * 3; ++i) tia1a.tick();

  mos6532.update(cpuCycles);
}

void Atari2600Bus::setInput(uint8_t swcha, uint8_t swchb, bool fire0,
                            bool fire1) {
  mos6532.setSwcha(swcha);
  mos6532.setSwchb(swchb);
  tia1a.setFire(0, fire0);
  tia1a.setFire(1, fire1);
}
