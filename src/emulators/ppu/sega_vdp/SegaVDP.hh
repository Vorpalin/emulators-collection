#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>

/**
 * @file SegaVDP.hh
 * @brief Sega Master System / Game Gear video display processor (Mode 4).
 */

/**
 * @struct SegaVDPState
 * @brief Serializable state of the VDP (the framebuffer is not saved: it is
 *        fully redrawn every frame).
 */
struct SegaVDPState {
  std::array<uint8_t, 0x4000> vram{};  ///< 16 KiB video RAM.
  std::array<uint8_t, 64> cram{};      ///< Colour RAM (32 bytes SMS, 64 GG).
  std::array<uint8_t, 16> reg{};       ///< VDP registers R0-R10 (+ spare).
  uint16_t address = 0;                ///< 14-bit VRAM/CRAM address.
  uint8_t code = 0;                    ///< Access code (0-3).
  bool latchPending = false;           ///< First control byte received.
  uint8_t latch = 0;                   ///< First control byte.
  uint8_t readBuffer = 0;              ///< Data port read-ahead buffer.
  uint8_t cramLatch = 0;               ///< Game Gear CRAM low-byte latch.
  bool vintPending = false;            ///< Frame interrupt flag (status b7).
  bool hintPending = false;            ///< Line interrupt pending.
  bool spriteOverflow = false;         ///< Status b6.
  bool spriteCollision = false;        ///< Status b5.
  int line = 0;                        ///< Current scanline.
  int lineCycles = 0;                  ///< CPU cycles elapsed in the line.
  int hintCounter = 0;                 ///< Line counter (R10 reload).
  uint8_t vscroll = 0;                 ///< Vertical scroll latched at frame start.
  bool pal = false;                    ///< 50 Hz timing (313 lines).
  bool gameGear = false;               ///< Game Gear VDP variant.
  bool irqLevel = false;               ///< Last level driven on /INT.
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SegaVDPState, vram, cram, reg, address, code,
                                   latchPending, latch, readBuffer, cramLatch,
                                   vintPending, hintPending, spriteOverflow,
                                   spriteCollision, line, lineCycles,
                                   hintCounter, vscroll, pal, gameGear,
                                   irqLevel)

/**
 * @class SegaVDP
 * @brief Scanline-rendered Sega VDP (TMS9918 derivative) running in Mode 4.
 *
 * Features: 32x28 (or 32x32) tile background with per-line horizontal scroll
 * and per-column vertical scroll locks, left-column mask, 64 sprites (8x8 /
 * 8x16, zoom, 8 per line with overflow and collision flags), tile priority,
 * 192/224/240-line modes, line and frame interrupts, V/H counters, NTSC and
 * PAL timing, and the Game Gear variant (12-bit palette, 160x144 viewport).
 *
 * Not emulated: the legacy TMS9918 modes 0-3 (shown as backdrop), mid-line
 * register changes (each line is drawn once, at its end, with the registers
 * of that moment), and the exact H counter / light-gun latch.
 *
 * Output is RGB555 per pixel (0bBBBBBGGGGGRRRRR), the same layout as the Game
 * Boy Color framebuffer. The /INT line (level-triggered, active when pending
 * and enabled) is reported through irqLine() and the interrupt callback.
 */
class SegaVDP {
 public:
  /** @brief VDP flavour. */
  enum class Model { MasterSystem, GameGear };

  static constexpr int kWidth = 256;        ///< Framebuffer width.
  static constexpr int kMaxHeight = 240;    ///< Framebuffer height.
  static constexpr int kCyclesPerLine = 228;  ///< Z80 cycles per scanline.

  /** @brief Constructs the VDP. @param model Master System or Game Gear. */
  explicit SegaVDP(Model model = Model::MasterSystem);

  /** @brief Resets registers, memories and timing (keeps model and region). */
  void reset();

  /** @brief Switches between Master System and Game Gear behaviour. */
  void setModel(Model model);
  /** @brief Selects 50 Hz (PAL, 313 lines) or 60 Hz (NTSC, 262 lines). */
  void setPAL(bool pal) { pal_ = pal; }

  /**
   * @brief Advances the VDP.
   * @param cpuCycles Z80 T-states elapsed (228 per scanline).
   */
  void tick(uint32_t cpuCycles);

  /// @name Port interface (SMS: 0xBE data, 0xBF control/status, 0x7E/0x7F)
  /// @{
  uint8_t readData();               ///< Port 0xBE read.
  void writeData(uint8_t value);    ///< Port 0xBE write.
  uint8_t readStatus();             ///< Port 0xBF read (clears flags/latch).
  void writeControl(uint8_t value); ///< Port 0xBF write.
  uint8_t readVCounter() const;     ///< Port 0x7E read.
  uint8_t readHCounter() const;     ///< Port 0x7F read (approximate).
  /// @}

  /** @brief Level of the /INT line (true = asserted). */
  bool irqLine() const { return irqLevel_; }

  /** @brief Callback invoked whenever the /INT level changes. */
  void onInterrupt(std::function<void(bool)> cb) { interruptCallback_ = cb; }
  /** @brief Callback invoked once per frame, when VBlank begins. */
  void onFrameReady(std::function<void()> cb) { frameReadyCallback_ = cb; }

  /** @brief Full 256x240 RGB555 framebuffer (only the visible part is valid). */
  const std::array<uint16_t, kWidth * kMaxHeight> &framebuffer() const {
    return fb_;
  }

  /** @brief Number of active lines: 192, 224 or 240. */
  int activeHeight() const;
  /// @name Visible area inside framebuffer()
  /// @{
  int viewportX() const { return gameGear_ ? 48 : 0; }
  int viewportY() const { return gameGear_ ? 24 : 0; }
  int viewportWidth() const { return gameGear_ ? 160 : 256; }
  int viewportHeight() const { return gameGear_ ? 144 : activeHeight(); }
  /// @}

  /** @brief Current scanline (0-based). */
  int currentLine() const { return line_; }
  /** @brief Read-only VRAM access (debugging, tests). */
  const std::array<uint8_t, 0x4000> &vram() const { return vram_; }

  /** @brief Gets the current state as a serializable structure. */
  SegaVDPState getState() const;
  /** @brief Restores the state from a serializable structure. */
  void setState(const SegaVDPState &state);

 private:
  std::array<uint8_t, 0x4000> vram_{};
  std::array<uint8_t, 64> cram_{};
  std::array<uint8_t, 16> reg_{};
  uint16_t address_ = 0;
  uint8_t code_ = 0;
  bool latchPending_ = false;
  uint8_t latch_ = 0;
  uint8_t readBuffer_ = 0;
  uint8_t cramLatch_ = 0;
  bool vintPending_ = false, hintPending_ = false;
  bool spriteOverflow_ = false, spriteCollision_ = false;
  int line_ = 0, lineCycles_ = 0, hintCounter_ = 0;
  uint8_t vscroll_ = 0;
  bool pal_ = false, gameGear_ = false, irqLevel_ = false;
  std::array<uint16_t, kWidth * kMaxHeight> fb_{};

  std::function<void(bool)> interruptCallback_;
  std::function<void()> frameReadyCallback_;

  int totalLines() const { return pal_ ? 313 : 262; }
  void writeRegister(int index, uint8_t value);
  void writeCram(uint16_t address, uint8_t value);
  uint16_t paletteColor(int index) const;
  void updateIrq();
  void endLine();
  void renderLine(int y);
};
