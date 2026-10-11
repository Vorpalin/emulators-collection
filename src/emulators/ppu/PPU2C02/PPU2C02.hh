#pragma once

#include <array>

constexpr int SCREEN_WIDTH = 256;   ///< LCD width in pixels.
constexpr int SCREEN_HEIGHT = 240;  ///< LCD height in pixels.

class PPU2C02 {
 public:
  PPU2C02();
  ~PPU2C02();

  void reset();
  void step(int cycles);

  uint8_t readRegister(uint16_t addr);
  void writeRegister(uint16_t addr, uint8_t value);

  const std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>& framebuffer() const {
    return framebuffer_;
  }

 private:
  std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer_;
};
