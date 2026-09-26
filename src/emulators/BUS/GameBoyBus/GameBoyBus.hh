#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "Bus.hh"
#include "Cartbridge.hh"
#include "GameBoyController.hh"
#include "GameBoyPPU.hh"
#include "GameBoyTimer.hh"
#include "InterruptController.hh"
#include "LR35902.hh"

class GameBoyBus : public Bus {
 public:
  GameBoyBus();

  void loadROM(std::string& filename) override;
  void reset() override;
  uint8_t readMemory(uint16_t address) override;
  void writeMemory(uint16_t address, uint8_t value) override;
  void tick() override;

  GameBoyController& getJoypad() { return joypad; }

 private:
  void startOamDma(uint8_t sourceHigh);

  LR35902 cpu;
  Cartbridge cartbridge;
  GameBoyPPU ppu;
  std::array<uint8_t, 0x2000> wram;  // Work RAM (0xC000 - 0xDFFF)
  std::array<uint8_t, 0x7F> hram;    // High RAM (0xFF80 - 0xFFFE)

  GameBoyTimer timer;
  GameBoyController joypad;
  InterruptController interrupts;

  uint8_t serialData = 0;
  uint8_t serialControl = 0;

  bool dmaActive = false;
};
