#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "emulators/cartridge/nes_cartridge/NesCartridge.hh"

/**
 * @file PPU2C02.hh
 * @brief NES picture processing unit (Ricoh 2C02), rendered dot by dot.
 *
 * The PPU is stepped one dot at a time (3 dots per CPU cycle). It draws the
 * background through the "loopy" v/t/x/w scroll registers and the sprites
 * through a per-scanline evaluation, and writes a palette-index framebuffer.
 */

/** One pending sprite on the current scanline. */
struct PpuSprite {
  uint8_t y = 0xFF;
  uint8_t tile = 0xFF;
  uint8_t attr = 0xFF;
  uint8_t x = 0xFF;
  uint8_t lo = 0;  ///< Pattern bits, plane 0 (shifted out MSB first).
  uint8_t hi = 0;  ///< Pattern bits, plane 1.
};

/** Serializable PPU state. */
struct PPU2C02State {
  uint8_t ctrl = 0;
  uint8_t mask = 0;
  uint8_t status = 0;
  uint8_t oam_addr = 0;
  uint16_t v = 0;
  uint16_t t = 0;
  uint8_t fine_x = 0;
  bool w = false;
  uint8_t read_buffer = 0;
  uint8_t io_latch = 0;

  int scanline = -1;
  int dot = 0;
  bool odd_frame = false;
  bool nmi_line = false;
  bool nmi_pending = false;
  bool frame_ready = false;

  uint8_t nt_byte = 0;
  uint8_t at_byte = 0;
  uint8_t pat_lo = 0;
  uint8_t pat_hi = 0;
  uint16_t bg_lo = 0;
  uint16_t bg_hi = 0;
  uint16_t at_lo = 0;
  uint16_t at_hi = 0;

  std::array<PpuSprite, 8> sprites;
  uint8_t sprite_count = 0;
  bool sprite0_in_line = false;

  bool a12 = false;
  uint64_t a12_low_since = 0;
  uint64_t dot_counter = 0;

  std::array<uint8_t, 4096> vram{};
  std::array<uint8_t, 32> palette{};
  std::array<uint8_t, 256> oam{};
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PpuSprite, y, tile, attr, x, lo, hi)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PPU2C02State, ctrl, mask, status, oam_addr,
                                   v, t, fine_x, w, read_buffer, io_latch,
                                   scanline, dot, odd_frame, nmi_line,
                                   nmi_pending, frame_ready, nt_byte, at_byte,
                                   pat_lo, pat_hi, bg_lo, bg_hi, at_lo, at_hi,
                                   sprites, sprite_count, sprite0_in_line, a12,
                                   a12_low_since, dot_counter, vram, palette,
                                   oam)

class PPU2C02 {
 public:
  static constexpr int kWidth = 256;
  static constexpr int kHeight = 240;

  /// Bits of the PPUCTRL register.
  enum Ctrl : uint8_t {
    kCtrlIncrement = 0x04,
    kCtrlSpritePattern = 0x08,
    kCtrlBgPattern = 0x10,
    kCtrlSprite16 = 0x20,
    kCtrlNmi = 0x80,
  };
  /// Bits of the PPUMASK register.
  enum Mask : uint8_t {
    kMaskGrey = 0x01,
    kMaskBgLeft = 0x02,
    kMaskSpritesLeft = 0x04,
    kMaskBg = 0x08,
    kMaskSprites = 0x10,
  };
  /// Bits of the PPUSTATUS register.
  enum Status : uint8_t {
    kStatusOverflow = 0x20,
    kStatusSprite0 = 0x40,
    kStatusVBlank = 0x80,
  };

  /// @param cartridge Cartridge providing CHR and mirroring (not owned).
  explicit PPU2C02(NesCartridge* cartridge);

  void reset();

  /// Advance the PPU by one dot.
  void step();

  /// Read a CPU-visible register ($2000-$3FFF, mirrors included).
  uint8_t readRegister(uint16_t addr);
  /// Write a CPU-visible register ($2000-$3FFF, mirrors included).
  void writeRegister(uint16_t addr, uint8_t value);

  /// One byte of an OAM DMA transfer (same effect as a write to $2004).
  void oamDmaWrite(uint8_t value) { writeRegister(0x2004, value); }

  /// True once per NMI edge; clears the pending flag.
  bool pollNmi() {
    const bool p = s_.nmi_pending;
    s_.nmi_pending = false;
    return p;
  }

  /// True once per completed frame (set when vertical blanking starts).
  bool consumeFrameReady() {
    const bool r = s_.frame_ready;
    s_.frame_ready = false;
    return r;
  }

  /// Color indices (bits 0-5) with the emphasis bits in bits 6-8.
  const std::array<uint16_t, kWidth * kHeight>& frameBuffer() const {
    return frame_;
  }

  const PPU2C02State& getState() const { return s_; }
  void setState(const PPU2C02State& s) { s_ = s; }

 private:
  uint8_t read(uint16_t addr);
  void write(uint16_t addr, uint8_t value);
  uint8_t fetchPattern(uint16_t addr);
  void trackA12(uint16_t addr);
  static uint8_t paletteIndex(uint16_t addr);

  bool renderingEnabled() const {
    return (s_.mask & (kMaskBg | kMaskSprites)) != 0;
  }
  void updateNmiLine();

  void incrementX();
  void incrementY();
  void copyX();
  void copyY();
  void loadBgShifters();
  void evaluateSprites();
  void fetchSprite(int slot, bool high);
  void renderPixel();

  PPU2C02State s_;
  NesCartridge* cart_;
  std::array<uint16_t, kWidth * kHeight> frame_{};
};
