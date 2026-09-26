#include "GameBoyBus.hh"

GameBoyBus::GameBoyBus() : cpu(this), cartbridge(), ppu(), wram(), hram() {}

GameBoyBus::~GameBoyBus() {}

void GameBoyBus::loadROM(std::string& filename) {
  cartbridge.loadROM(filename);
}

void GameBoyBus::reset() {
  cpu.reset();
  cartbridge.reset();
  ppu.reset();
  timer.reset();
  joypad.reset();
  interrupts.reset();
  wram.fill(0);
  hram.fill(0);
  serialData = 0;
  serialControl = 0;
  dmaActive = false;
}

void GameBoyBus::tick() {
  // For now, we will just execute one instruction per tick.
  // In a real implementation, we would need to handle timing and
  // synchronization with other components like the GPU and APU.
  uint32_t cycles = cpu.execute();
  ppu.tick(cycles);
  timer.tick(cycles);
}

void GameBoyBus::writeMemory(uint16_t address, uint8_t value) {
  if (address < 0x8000) {
    cartbridge.write(address, value);
  } else if (address < 0xA000) {
    ppu.writeMemory(address, value);  // VRAM
  } else if (address < 0xC000) {
    cartbridge.write(address, value);  // external/save RAM
  } else if (address < 0xE000) {
    wram[address - 0xC000] = value;
  } else if (address < 0xFE00) {
    wram[address - 0x2000] = value;  // echo RAM mirrors 0xC000-0xDDFF
  } else if (address < 0xFEA0) {
    ppu.writeMemory(address, value);  // OAM
  } else if (address < 0xFF00) {
    // Unusable region; real hardware mostly ignores writes here.
  } else if (address < 0xFF80) {
    switch (address) {
      case 0xFF00:
        joypad.write(value);
        break;
      case 0xFF01:
        serialData = value;
        break;
      case 0xFF02:
        serialControl = value;
        break;
      case 0xFF04:
      case 0xFF05:
      case 0xFF06:
      case 0xFF07:
        timer.write(address, value);
        break;
      case 0xFF0F:
        interrupts.writeIF(value);
        break;
      case 0xFF46:
        startOamDma(value);
        break;
      default:
        ppu.writeMemory(address, value);
        break;  // APU/PPU registers
    }
  } else if (address < 0xFFFF) {
    hram[address - 0xFF80] = value;
  } else {
    interrupts.writeIE(value);
  }
}

uint8_t GameBoyBus::readMemory(uint16_t address) {
  if (address < 0x8000) {
    return cartbridge.read(address);
  } else if (address < 0xA000) {
    return ppu.readMemory(address);  // VRAM
  } else if (address < 0xC000) {
    return cartbridge.read(address);  // external/save RAM
  } else if (address < 0xE000) {
    return wram[address - 0xC000];
  } else if (address < 0xFE00) {
    return wram[address - 0x2000];  // echo RAM
  } else if (address < 0xFEA0) {
    return ppu.readMemory(address);  // OAM
  } else if (address < 0xFF00) {
    return 0xFF;  // unusable region
  } else if (address < 0xFF80) {
    switch (address) {
      case 0xFF00:
        return joypad.read();
      case 0xFF01:
        return serialData;
      case 0xFF02:
        return serialControl;
      case 0xFF04:
      case 0xFF05:
      case 0xFF06:
      case 0xFF07:
        return timer.read(address);
      case 0xFF0F:
        return interrupts.readIF();
      default:
        return ppu.readMemory(address);  // APU/PPU registers
    }
  } else if (address < 0xFFFF) {
    return hram[address - 0xFF80];
  } else {
    return interrupts.readIE();
  }
}

void GameBoyBus::startOamDma(uint8_t sourceHigh) {
  uint16_t base = static_cast<uint16_t>(sourceHigh) << 8;
  for (uint16_t i = 0; i < 0xA0; ++i) {
    ppu.writeMemory(0xFE00 + i, readMemory(base + i));
  }
}
