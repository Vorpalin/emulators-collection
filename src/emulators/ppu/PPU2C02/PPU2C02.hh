#pragma once

#include <array>
#include <cstdint>
#include <utility>

#include "emulators/mirroring/nes_mirroring/NesMirroring.hh"

constexpr int SCREEN_WIDTH = 256;   ///< LCD width in pixels.
constexpr int SCREEN_HEIGHT = 240;  ///< LCD height in pixels.

// 7  bit  0
// ---- ----
// VPHB SINN
// |||| ||||
// |||| ||++- Base nametable address
// |||| ||    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
// |||| |+--- VRAM address increment per CPU read/write of PPUDATA
// |||| |     (0: add 1, going across; 1: add 32, going down)
// |||| +---- Sprite pattern table address for 8x8 sprites
// ||||       (0: $0000; 1: $1000; ignored in 8x16 mode)
// |||+------ Background pattern table address (0: $0000; 1: $1000)
// ||+------- Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
// |+-------- PPU master/slave select
// |          (0: read backdrop from EXT pins; 1: output color on EXT pins)
// +--------- Generate an NMI at the start of the
//            vertical blanking interval (0: off; 1: on)
class ControlRegister {
 public:
  enum Flag : uint8_t {
    NAMETABLE1 = 0b00000001,
    NAMETABLE2 = 0b00000010,
    VRAM_ADD_INCREMENT = 0b00000100,
    SPRITE_PATTERN_ADDR = 0b00001000,
    BACKROUND_PATTERN_ADDR = 0b00010000,
    SPRITE_SIZE = 0b00100000,
    MASTER_SLAVE_SELECT = 0b01000000,
    GENERATE_NMI = 0b10000000
  };

  ControlRegister() : bits_(0) {}

  bool contains(Flag flag) const { return (bits_ & flag) == flag; }

  uint8_t vram_addr_increment() const {
    return contains(VRAM_ADD_INCREMENT) ? 32 : 1;
  }

  void update(uint8_t data) { bits_ = data; }

  uint8_t bits() const { return bits_; }

 private:
  uint8_t bits_;
};

class AddrRegister {
 public:
  AddrRegister() : value_(0, 0), hi_ptr_(true) {}

  void update(uint8_t data) {
    if (hi_ptr_) {
      value_.first = data;
    } else {
      value_.second = data;
    }

    if (get() > 0x3fff) {  // mirror down addr above 0x3fff
      set(get() & 0x3fff);
    }
    hi_ptr_ = !hi_ptr_;
  }

  void increment(uint8_t inc) {
    uint8_t lo = value_.second;
    value_.second = static_cast<uint8_t>(value_.second + inc);
    if (lo > value_.second) {
      value_.first = static_cast<uint8_t>(value_.first + 1);
    }
    if (get() > 0x3fff) {
      set(get() & 0x3fff);  // mirror down addr above 0x3fff
    }
  }

  void reset_latch() { hi_ptr_ = true; }

  uint16_t get() const {
    return (static_cast<uint16_t>(value_.first) << 8) |
           static_cast<uint16_t>(value_.second);
  }

 private:
  void set(uint16_t data) {
    value_.first = static_cast<uint8_t>(data >> 8);
    value_.second = static_cast<uint8_t>(data & 0xff);
  }

  std::pair<uint8_t, uint8_t> value_;
  bool hi_ptr_;
};

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

  void writePPUAddr(uint8_t data);
  void writePPUCtrl(uint8_t data);

  void incrementVramAddr();

  uint8_t readPPUData();

 private:
  std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>
      chr_rom_;                      ///< Character ROM (pattern tables).
  std::array<uint8_t, 32> palette_;  ///< Palette RAM (color information).
  std::array<uint8_t, 2048>
      vram_;  ///< Video RAM (name tables and attribute tables).
  std::array<uint8_t, 256> oam_data_;
  NesMirroring mirroring_;   ///< Mirroring type for name tables.
  AddrRegister vram_addr_;   ///< Current VRAM address (15 bits).
  ControlRegister control_;  ///< Control register (PPUCTRL).

  uint8_t internal_buffer_;  ///< Internal buffer for PPU data reads.

  uint16_t mirrorVramAddr(uint16_t addr);
};
