#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "Bus.hh"
#include "Cartbridge.hh"
#include "GameBoyPPU.hh"
#include "LR35902.hh"

class GameBoyBus : public Bus {
 public:
  GameBoyBus();

  void loadROM(std::string& filename) override;
  void reset() override;
  uint8_t readMemory(uint16_t address) override;
  void writeMemory(uint16_t address, uint8_t value) override;
  void tick() override;

 private:
  LR35902 cpu;
  Cartbridge cartbridge;
  GameBoyPPU ppu;
};
