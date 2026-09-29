#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "mode.hh"

constexpr int SCREEN_WIDTH = 160;
constexpr int SCREEN_HEIGHT = 144;

struct PPURegisters {
  uint8_t lcdc = 0x91;  // 0xFF40 - LCD Control
  uint8_t stat = 0x85;  // 0xFF41 - LCD Status
  uint8_t scy = 0x00;   // 0xFF42 - Scroll Y
  uint8_t scx = 0x00;   // 0xFF43 - Scroll X
  uint8_t ly = 0x00;    // 0xFF44 - Current Scanline
  uint8_t lyc = 0x00;   // 0xFF45 - LY Compare
  uint8_t dma = 0x00;   // 0xFF46 - DMA Transfer
  uint8_t bgp = 0xFC;   // 0xFF47 - Palette Background
  uint8_t obp0 = 0xFF;  // 0xFF48 - Palette sprites 0
  uint8_t obp1 = 0xFF;  // 0xFF49 - Palette sprites 1
  uint8_t wy = 0x00;    // 0xFF4A - Y Position of the window
  uint8_t wx = 0x00;    // 0xFF4B - X Position of the window

  bool lcdEnabled() const { return lcdc & 0x80; }
  bool windowTileMapHigh() const { return lcdc & 0x40; }
  bool windowEnabled() const { return lcdc & 0x20; }
  bool bgWindowDataLow() const { return lcdc & 0x10; }
  bool bgTileMapHigh() const { return lcdc & 0x08; }
  bool spriteSize8x16() const { return lcdc & 0x04; }
  bool spritesEnabled() const { return lcdc & 0x02; }
  bool bgWindowEnabled() const { return lcdc & 0x01; }

  Mode mode() const { return static_cast<Mode>(stat & 0x03); }
  void setMode(Mode m) {
    stat = static_cast<uint8_t>((stat & 0xFC) | static_cast<uint8_t>(m));
  }
  bool lycInterruptEnabled() const { return stat & 0x40; }
  bool oamInterruptEnabled() const { return stat & 0x20; }
  bool vblankInterruptEnabled() const { return stat & 0x10; }
  bool hblankInterruptEnabled() const { return stat & 0x08; }
  bool coincidenceFlag() const { return stat & 0x04; }
};

struct Sprite {
  uint8_t y;           // Position Y (- 16)
  uint8_t x;           // Position X (- 8)
  uint8_t tileIndex;   // Tile index in VRAM
  uint8_t attributes;  // Flags (priority, flip, palette)

  bool priorityBehindBG() const { return attributes & 0x80; }
  bool flipY() const { return attributes & 0x40; }
  bool flipX() const { return attributes & 0x20; }
  bool paletteOBP1() const { return attributes & 0x10; }
};

class GameBoyPPU {
 public:
  GameBoyPPU();
  void reset();
  void tick(uint8_t cycles);
  uint8_t readMemory(uint16_t address);
  void writeMemory(uint16_t address, uint8_t value);

  uint8_t readVRAM(uint16_t addr) const;
  void writeVRAM(uint16_t addr, uint8_t value);

  uint8_t readOAM(uint16_t addr) const;
  void writeOAM(uint16_t addr, uint8_t value);

  uint8_t readRegister(uint16_t addr) const;
  void writeRegister(uint16_t addr, uint8_t value);

  const std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>& framebuffer() const {
    return framebuffer_;
  }

  void onFrameReady(std::function<void()> callback) {
    frameReadyCallback_ = callback;
  }

  // Callbacks d'interruption (à brancher sur le CPU / contrôleur
  // d'interruptions)
  void onVBlankInterrupt(std::function<void()> cb) { vblankInterrupt_ = cb; }
  void onStatInterrupt(std::function<void()> cb) { statInterrupt_ = cb; }

  const PPURegisters& registers() const { return regs_; }

 private:
  PPURegisters regs_;
  std::array<uint8_t, 0x2000> vram;  // Video RAM (0x8000 - 0x9FFF)
  std::array<uint8_t, 0xA0> oam;     // Sprite Attribute Table (0xFE00 - 0xFE9F)
  std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT> framebuffer_{};
  int dotCounter_ = 0;  // Compteur de cycles dans la ligne courante (0-455)
  std::function<void()> frameReadyCallback_;
  std::function<void()> vblankInterrupt_;
  std::function<void()> statInterrupt_;

  void changeMode(Mode newMode);
  void renderScanline();
  void renderBackgroundLine(int line);
  void renderWindowLine(int line);
  void renderSpritesLine(int line);
  void checkLYC();

  uint8_t getColorFromPalette(uint8_t paletteReg, uint8_t colorIndex) const;
  uint8_t readTilePixel(uint16_t tileDataAddr, int tileX, int tileY) const;
};
