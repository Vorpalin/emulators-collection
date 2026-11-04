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

inline void to_json(nlohmann::json& j, const PpuSprite& v) {
  j = nlohmann::json{{"y", v.y}, {"tile", v.tile}, {"attr", v.attr},
                     {"x", v.x}, {"lo", v.lo},     {"hi", v.hi}};
}

inline void from_json(const nlohmann::json& j, PpuSprite& v) {
  j.at("y").get_to(v.y);
  j.at("tile").get_to(v.tile);
  j.at("attr").get_to(v.attr);
  j.at("x").get_to(v.x);
  j.at("lo").get_to(v.lo);
  j.at("hi").get_to(v.hi);
}
inline void to_json(nlohmann::json& j, const PPU2C02State& v) {
  j = nlohmann::json{{"ctrl", v.ctrl},
                     {"mask", v.mask},
                     {"status", v.status},
                     {"oam_addr", v.oam_addr},
                     {"v", v.v},
                     {"t", v.t},
                     {"fine_x", v.fine_x},
                     {"w", v.w},
                     {"read_buffer", v.read_buffer},
                     {"io_latch", v.io_latch},
                     {"scanline", v.scanline},
                     {"dot", v.dot},
                     {"odd_frame", v.odd_frame},
                     {"nmi_line", v.nmi_line},
                     {"nmi_pending", v.nmi_pending},
                     {"frame_ready", v.frame_ready},
                     {"nt_byte", v.nt_byte},
                     {"at_byte", v.at_byte},
                     {"pat_lo", v.pat_lo},
                     {"pat_hi", v.pat_hi},
                     {"bg_lo", v.bg_lo},
                     {"bg_hi", v.bg_hi},
                     {"at_lo", v.at_lo},
                     {"at_hi", v.at_hi},
                     {"sprites", v.sprites},
                     {"sprite_count", v.sprite_count},
                     {"sprite0_in_line", v.sprite0_in_line},
                     {"a12", v.a12},
                     {"a12_low_since", v.a12_low_since},
                     {"dot_counter", v.dot_counter},
                     {"vram", v.vram},
                     {"palette", v.palette},
                     {"oam", v.oam}};
}

inline void from_json(const nlohmann::json& j, PPU2C02State& v) {
  j.at("ctrl").get_to(v.ctrl);
  j.at("mask").get_to(v.mask);
  j.at("status").get_to(v.status);
  j.at("oam_addr").get_to(v.oam_addr);
  j.at("v").get_to(v.v);
  j.at("t").get_to(v.t);
  j.at("fine_x").get_to(v.fine_x);
  j.at("w").get_to(v.w);
  j.at("read_buffer").get_to(v.read_buffer);
  j.at("io_latch").get_to(v.io_latch);
  j.at("scanline").get_to(v.scanline);
  j.at("dot").get_to(v.dot);
  j.at("odd_frame").get_to(v.odd_frame);
  j.at("nmi_line").get_to(v.nmi_line);
  j.at("nmi_pending").get_to(v.nmi_pending);
  j.at("frame_ready").get_to(v.frame_ready);
  j.at("nt_byte").get_to(v.nt_byte);
  j.at("at_byte").get_to(v.at_byte);
  j.at("pat_lo").get_to(v.pat_lo);
  j.at("pat_hi").get_to(v.pat_hi);
  j.at("bg_lo").get_to(v.bg_lo);
  j.at("bg_hi").get_to(v.bg_hi);
  j.at("at_lo").get_to(v.at_lo);
  j.at("at_hi").get_to(v.at_hi);
  j.at("sprites").get_to(v.sprites);
  j.at("sprite_count").get_to(v.sprite_count);
  j.at("sprite0_in_line").get_to(v.sprite0_in_line);
  j.at("a12").get_to(v.a12);
  j.at("a12_low_since").get_to(v.a12_low_since);
  j.at("dot_counter").get_to(v.dot_counter);
  j.at("vram").get_to(v.vram);
  j.at("palette").get_to(v.palette);
  j.at("oam").get_to(v.oam);
}

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
