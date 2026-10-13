#pragma once

#include <array>

#include "emulators/mirroring/nes_mirroring/NesMirroring.hh"

constexpr int SCREEN_WIDTH = 256;   ///< LCD width in pixels.
constexpr int SCREEN_HEIGHT = 240;  ///< LCD height in pixels.

class PPU2C02 {
 public:
  PPU2C02(NesMirroring mirroring);
  ~PPU2C02();

  void reset();
  void step(int cycles);

  uint8_t readRegister(uint16_t addr);
  void writeRegister(uint16_t addr, uint8_t value);

  const std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>& getFrameBuffer()
      const {
    return chr_rom_;
  }

 private:
  std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>
      chr_rom_;                      ///< Character ROM (pattern tables).
  std::array<uint8_t, 32> palette_;  ///< Palette RAM (color information).
  std::array<uint8_t, 2048>
      vram_;  ///< Video RAM (name tables and attribute tables).
  std::array<uint8_t, 256> oam_data_;
  NesMirroring mirroring_;  ///< Mirroring type for name tables.
};
