#pragma once
#include <string>

class Bus {
 public:
  virtual void loadROM(std::string &filename) = 0;

  virtual void reset() = 0;

  virtual void tick() = 0;

  virtual uint8_t readMemory(uint16_t address) = 0;
  virtual void writeMemory(uint16_t address, uint8_t value) = 0;
};
