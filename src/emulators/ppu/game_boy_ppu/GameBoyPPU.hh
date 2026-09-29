#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>

#include "emulators/ppu/game_boy_ppu/mode.hh"

/**
 * @file GameBoyPPU.hh
 * @brief Game Boy Picture Processing Unit (LCD controller) emulation.
 */

constexpr int SCREEN_WIDTH = 160;   ///< LCD width in pixels.
constexpr int SCREEN_HEIGHT = 144;  ///< LCD height in pixels.

/**
 * @struct PPURegisters
 * @brief PPU I/O registers (0xFF40-0xFF4B) with bit-field helpers.
 *
 * Default values correspond to the state after the boot ROM has run.
 */
struct PPURegisters {
  uint8_t lcdc = 0x91;  ///< 0xFF40 - LCD Control
  uint8_t stat = 0x85;  ///< 0xFF41 - LCD Status
  uint8_t scy = 0x00;   ///< 0xFF42 - Scroll Y
  uint8_t scx = 0x00;   ///< 0xFF43 - Scroll X
  uint8_t ly = 0x00;    ///< 0xFF44 - Current scanline
  uint8_t lyc = 0x00;   ///< 0xFF45 - LY compare
  uint8_t dma = 0x00;   ///< 0xFF46 - OAM DMA source/start
  uint8_t bgp = 0xFC;   ///< 0xFF47 - Background palette
  uint8_t obp0 = 0xFF;  ///< 0xFF48 - Sprite palette 0
  uint8_t obp1 = 0xFF;  ///< 0xFF49 - Sprite palette 1
  uint8_t wy = 0x00;    ///< 0xFF4A - Window Y position
  uint8_t wx = 0x00;    ///< 0xFF4B - Window X position (+7)

  /// @name LCDC bit accessors
  /// @{
  bool lcdEnabled() const { return lcdc & 0x80; }  ///< Bit 7: LCD on.
  bool windowTileMapHigh() const {
    return lcdc & 0x40;
  }  ///< Bit 6: window map at 0x9C00.
  bool windowEnabled() const { return lcdc & 0x20; }  ///< Bit 5: window on.
  bool bgWindowDataLow() const {
    return lcdc & 0x10;
  }  ///< Bit 4: BG/window tile data at 0x8000 (unsigned indices).
  bool bgTileMapHigh() const {
    return lcdc & 0x08;
  }  ///< Bit 3: BG map at 0x9C00.
  bool spriteSize8x16() const { return lcdc & 0x04; }  ///< Bit 2: 8x16 sprites.
  bool spritesEnabled() const { return lcdc & 0x02; }  ///< Bit 1: sprites on.
  bool bgWindowEnabled() const {
    return lcdc & 0x01;
  }  ///< Bit 0: BG/window on.
  /// @}

  /// @name STAT accessors
  /// @{
  /** @brief Current PPU mode (STAT bits 0-1). */
  Mode mode() const { return static_cast<Mode>(stat & 0x03); }
  /** @brief Sets the PPU mode bits (STAT bits 0-1), leaving other bits intact.
   */
  void setMode(Mode m) {
    stat = static_cast<uint8_t>((stat & 0xFC) | static_cast<uint8_t>(m));
  }
  bool lycInterruptEnabled() const {
    return stat & 0x40;
  }  ///< Bit 6: LYC=LY STAT interrupt.
  bool oamInterruptEnabled() const {
    return stat & 0x20;
  }  ///< Bit 5: mode 2 (OAM) STAT interrupt.
  bool vblankInterruptEnabled() const {
    return stat & 0x10;
  }  ///< Bit 4: mode 1 (VBlank) STAT interrupt.
  bool hblankInterruptEnabled() const {
    return stat & 0x08;
  }  ///< Bit 3: mode 0 (HBlank) STAT interrupt.
  bool coincidenceFlag() const { return stat & 0x04; }  ///< Bit 2: LY == LYC.
  /// @}
};

/**
 * @struct Sprite
 * @brief One 4-byte OAM entry.
 */
struct Sprite {
  uint8_t y;           ///< Y position + 16 (on-screen Y = y - 16).
  uint8_t x;           ///< X position + 8 (on-screen X = x - 8).
  uint8_t tileIndex;   ///< Tile index in VRAM.
  uint8_t attributes;  ///< Flags (priority, flip, palette).

  bool priorityBehindBG() const {
    return attributes & 0x80;
  }  ///< Bit 7: drawn behind BG colors 1-3.
  bool flipY() const { return attributes & 0x40; }  ///< Bit 6: vertical flip.
  bool flipX() const { return attributes & 0x20; }  ///< Bit 5: horizontal flip.
  bool paletteOBP1() const {
    return attributes & 0x10;
  }  ///< Bit 4: uses OBP1 instead of OBP0.
};

/**
 * @class GameBoyPPU
 * @brief Scanline-based PPU: VRAM, OAM, LCD registers and rendering.
 *
 * Each scanline lasts 456 dots and goes through OAM scan, drawing and
 * HBlank; lines 144-153 are VBlank. The PPU renders each scanline
 * (background, window, sprites) into a 160x144 framebuffer, and notifies the
 * rest of the system through callbacks (frame ready, VBlank and STAT
 * interrupts).
 */
class GameBoyPPU {
 public:
  /** @brief Constructs the PPU in its post-boot state. */
  GameBoyPPU();

  /** @brief Resets registers, memories, framebuffer and timing. */
  void reset();

  /**
   * @brief Advances the PPU state machine.
   * @param cycles Number of dots (PPU clock cycles) elapsed.
   */
  void tick(uint8_t cycles);

  /**
   * @brief Reads from the PPU-owned address space (VRAM, OAM, registers).
   * @param address Bus address.
   * @return Byte read.
   */
  uint8_t read(uint16_t address);

  /**
   * @brief Writes to the PPU-owned address space (VRAM, OAM, registers).
   * @param address Bus address.
   * @param value   Byte to write.
   */
  void write(uint16_t address, uint8_t value);

  /** @brief Reads VRAM. @param addr Bus address in 0x8000-0x9FFF. */
  uint8_t readVRAM(uint16_t addr) const;
  /** @brief Writes VRAM. @param addr Bus address in 0x8000-0x9FFF. @param value
   * Byte to write. */
  void writeVRAM(uint16_t addr, uint8_t value);

  /** @brief Reads OAM. @param addr Bus address in 0xFE00-0xFE9F. */
  uint8_t readOAM(uint16_t addr) const;
  /** @brief Writes OAM. @param addr Bus address in 0xFE00-0xFE9F. @param value
   * Byte to write. */
  void writeOAM(uint16_t addr, uint8_t value);

  /** @brief Reads a PPU register. @param addr Bus address in 0xFF40-0xFF4B. */
  uint8_t readRegister(uint16_t addr) const;
  /** @brief Writes a PPU register. @param addr Bus address in 0xFF40-0xFF4B.
   * @param value Byte to write. */
  void writeRegister(uint16_t addr, uint8_t value);

  /**
   * @brief Returns the current framebuffer.
   * @return 160x144 array, one byte per pixel (palette-resolved shade).
   */
  const std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>& framebuffer() const {
    return framebuffer_;
  }

  /**
   * @brief Registers a callback invoked when a full frame has been rendered.
   * @param callback Function to call at the start of VBlank.
   */
  void onFrameReady(std::function<void()> callback) {
    frameReadyCallback_ = callback;
  }

  /// @name Interrupt callbacks
  /// To be wired to the CPU / interrupt controller.
  /// @{
  /** @brief Registers the callback raised for the VBlank interrupt. */
  void onVBlankInterrupt(std::function<void()> cb) { vblankInterrupt_ = cb; }
  /** @brief Registers the callback raised for the STAT interrupt. */
  void onStatInterrupt(std::function<void()> cb) { statInterrupt_ = cb; }
  /// @}

  /** @brief Read-only access to the PPU registers. */
  const PPURegisters& registers() const { return regs_; }

 private:
  PPURegisters regs_;                ///< LCD registers.
  std::array<uint8_t, 0x2000> vram;  ///< Video RAM (0x8000-0x9FFF).
  std::array<uint8_t, 0xA0> oam;  ///< Sprite attribute table (0xFE00-0xFE9F).
  std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT>
      framebuffer_{};   ///< Output pixels.
  int dotCounter_ = 0;  ///< Dot counter within the current line (0-455).
  std::function<void()>
      frameReadyCallback_;  ///< Called when a frame is complete.
  std::function<void()>
      vblankInterrupt_;  ///< Called to request a VBlank interrupt.
  std::function<void()>
      statInterrupt_;  ///< Called to request a STAT interrupt.

  /** @brief Switches PPU mode, updates STAT and fires STAT interrupts if
   * enabled. */
  void changeMode(Mode newMode);
  /** @brief Renders the current scanline (BG, window, then sprites). */
  void renderScanline();
  /** @brief Renders the background for one line. @param line Scanline (0-143).
   */
  void renderBackgroundLine(int line);
  /** @brief Renders the window layer for one line. @param line Scanline
   * (0-143). */
  void renderWindowLine(int line);
  /** @brief Renders visible sprites for one line. @param line Scanline (0-143).
   */
  void renderSpritesLine(int line);
  /** @brief Compares LY with LYC, updates the coincidence flag and STAT
   * interrupt. */
  void checkLYC();

  /**
   * @brief Maps a 2-bit color index through a palette register.
   * @param paletteReg BGP, OBP0 or OBP1 value.
   * @param colorIndex Color index (0-3).
   * @return Resulting shade (0-3).
   */
  uint8_t getColorFromPalette(uint8_t paletteReg, uint8_t colorIndex) const;

  /**
   * @brief Decodes one pixel of a tile.
   * @param tileDataAddr VRAM address of the tile's first byte.
   * @param tileX Pixel column within the tile (0-7).
   * @param tileY Pixel row within the tile (0-7).
   * @return 2-bit color index (0-3).
   */
  uint8_t readTilePixel(uint16_t tileDataAddr, int tileX, int tileY) const;
};
