#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Cartbridge {
 public:
  Cartbridge();

  void loadROM(std::string &filename);
  void reset();

  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);

 private:
  std::vector<uint8_t> romData;  // Vector to hold the ROM data
  uint8_t bank = 0;
};
