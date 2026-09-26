#include "GameBoyBus.hh"

GameBoyBus::GameBoyBus() : cpu(this), cartbridge(), ppu() {}

GameBoyBus::~GameBoyBus() {}

void GameBoyBus::loadROM(std::string& filename) {
  cartbridge.loadROM(filename);
}

void GameBoyBus::reset() {
  cpu.reset();
  cartbridge.reset();
  ppu.reset();
}

void GameBoyBus::tick() {
  // For now, we will just execute one instruction per tick.
  // In a real implementation, we would need to handle timing and
  // synchronization with other components like the GPU and APU.
  uint32_t cycles = cpu.execute();
  ppu.tick(cycles);
}

void GameBoyBus::writeMemory(uint16_t address, uint8_t value) {
  if (address < 0x8000) {
    // Write to cartridge address space
    cartbridge.write(address, value);
  } else {
    ppu.writeMemory(address, value);
  }
}

uint8_t GameBoyBus::readMemory(uint16_t address) {
  if (address < 0x8000) {
    // Read from cartridge address space
    return cartbridge.read(address);
  } else {
    return ppu.readMemory(address);
  }
}
