#pragma once
#include <array>
#include <cstdint>

/**
 * @file TIA1A.hh
 * @brief TIA (Television Interface Adaptor) video emulation for the
 *        Atari 2600: playfield, players, missiles, ball and collision
 *        detection, rendered line-by-line into a framebuffer.
 */

/**
 * @class TIA1A
 * @brief Emulates the Atari 2600's TIA video chip. Advances one color
 *        clock at a time via tick(), accepts CPU register writes/reads,
 *        and produces a full NTSC-timed frame buffer.
 */
class TIA1A {
 public:
  static constexpr int kWidth = 160;   ///< Visible pixels per scanline.
  static constexpr int kHeight = 312;  ///< Total scanlines per frame (NTSC).
  static constexpr int kLineClocks = 228;  ///< Color clocks per scanline.
  static constexpr int kHBlank =
      68;  ///< Horizontal blank clocks at the start of each line.

  /**
   * @brief Construct the TIA and reset it to its power-up state.
   */
  TIA1A() { reset(); }

  /**
   * @brief Reset all TIA registers and internal state.
   */
  void reset();

  /**
   * @brief Advance the TIA by one color clock, updating beam position,
   *        rendering the current pixel when appropriate, and tracking
   *        frame completion.
   */
  void tick();

  /**
   * @brief Handle a CPU write to a TIA register.
   * @param addr  Register address (TIA address space).
   * @param value Byte value written.
   */
  void write(uint16_t addr, uint8_t value);

  /**
   * @brief Handle a CPU read from a TIA register (e.g. collision latches,
   *        input ports).
   * @param addr Register address (TIA address space).
   * @return The byte value read.
   */
  uint8_t read(uint16_t addr) const;

  /**
   * @brief Whether the CPU should currently be halted, e.g. due to WSYNC.
   * @return True if the CPU is halted.
   */
  bool cpuHalted() const;

  /**
   * @brief Whether a full frame has finished rendering and is ready to
   *        be displayed.
   * @return True if a new frame is available in frame().
   */
  bool frameReady() const;

  /**
   * @brief Clear the frame-ready flag after the caller has consumed the
   *        frame buffer.
   */
  void clearFrameReady();

  /**
   * @brief Access the rendered frame buffer.
   * @return Pointer to kWidth * kHeight color-index pixels.
   */
  const uint8_t* frame() const;

  /**
   * @brief Set the fire-button state for a player's joystick input.
   * @param player Player index (0 or 1).
   * @param pressed True if the fire button is currently pressed.
   */
  void setFire(int player, bool pressed);

 private:
  /// @brief Per-player sprite state (player graphics register).
  struct Player {
    uint8_t grp = 0, grpOld = 0,
            nusiz = 0;  ///< Graphics bitmap, delayed copy, size/copies config.
    bool reflect = false,
         vdel = false;  ///< Horizontal mirror flag, vertical-delay flag.
    int pos = 0;        ///< Horizontal position counter.
    int8_t hm = 0;      ///< Horizontal motion value.
  };
  /// @brief Per-missile sprite state.
  struct Missile {
    bool enabled = false,
         resetToPlayer =
             false;  ///< Enable flag, "lock to player position" flag.
    int pos = 0;     ///< Horizontal position counter.
    int8_t hm = 0;   ///< Horizontal motion value.
  };
  /// @brief Ball sprite state.
  struct Ball {
    bool enabled = false, enabledOld = false,
         vdel = false;  ///< Enable flag, delayed copy, vertical-delay flag.
    int pos = 0;        ///< Horizontal position counter.
    int8_t hm = 0;      ///< Horizontal motion value.
  };

  /// @brief Bit indices into the collision latch register (cx_).
  enum Cx {
    M0P1,
    M0P0,
    M1P0,
    M1P1,
    P0PF,
    P0BL,
    P1PF,
    P1BL,
    M0PF,
    M0BL,
    M1PF,
    M1BL,
    BLPF,
    P0P1,
    M0M1
  };

  /**
   * @brief Compute the color index of the pixel at horizontal position x,
   *        resolving sprite/playfield priority, and update collision bits.
   * @param x Horizontal beam position (0..kWidth-1).
   * @return Color index for the pixel.
   */
  uint8_t renderPixel(int x);

  /**
   * @brief Test whether the playfield is set at horizontal position x.
   * @param x Horizontal beam position.
   * @return True if the playfield bit is set at x.
   */
  bool playfieldBit(int x) const;

  /**
   * @brief Test whether a player sprite covers horizontal position x.
   * @param p Player state to test.
   * @param x Horizontal beam position.
   * @return True if the player's graphics bitmap is set at x.
   */
  bool playerPixel(const Player& p, int x) const;

  /**
   * @brief Test whether a missile sprite covers horizontal position x.
   * @param m Missile state to test.
   * @param p Associated player, used when the missile is locked to it.
   * @param x Horizontal beam position.
   * @return True if the missile is visible at x.
   */
  bool missilePixel(const Missile& m, const Player& p, int x) const;

  /**
   * @brief Test whether the ball sprite covers horizontal position x.
   * @param x Horizontal beam position.
   * @return True if the ball is visible at x.
   */
  bool ballPixel(int x) const;

  /**
   * @brief Apply the extra-clock horizontal motion (HMOVE) adjustment to
   *        all sprite position counters.
   */
  void applyHmove();

  Player p_[2];   ///< Player 0 and Player 1 sprite state.
  Missile m_[2];  ///< Missile 0 and Missile 1 sprite state.
  Ball bl_;       ///< Ball sprite state.

  uint8_t pf0_ = 0, pf1_ = 0, pf2_ = 0,
          ctrlpf_ = 0;  ///< Playfield graphics registers and control.
  uint8_t colup_[2] = {0, 0}, colupf_ = 0,
          colubk_ = 0;  ///< Player, playfield and background colors.
  uint16_t cx_ = 0;     ///< Collision latch bits, indexed by Cx.

  bool vsync_ = false, vblank_ = false,
       wsync_ = false;  ///< Sync/blank/CPU-halt latches.
  bool hmoveBlank_ = false,
       frameReady_ = false;  ///< HMOVE blanking flag, frame-complete flag.
  bool fire_[2] = {false, false};  ///< Fire-button state per player.

  int hpos_ = 0, vpos_ = 0;  ///< Current horizontal/vertical beam position.
  std::array<uint8_t, kWidth * kHeight> frame_{};  ///< Rendered frame buffer.
};
