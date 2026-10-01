#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "emulators/console/Console.hh"

/**
 * @file Chip8.hh
 * @brief CHIP-8 / Super-CHIP interpreter, implementing the Console interface.
 */

/**
 * @class Chip8
 * @brief Emulates a CHIP-8 system: 4K memory, 16 general registers, a
 *        64x32 (or 128x64 in high-resolution mode) monochrome display,
 *        a simple call stack, timers and a hex keypad.
 *
 * This class does not depend on SDL: video is exposed as an RGBA
 * framebuffer, audio as float PCM samples, and input is received through
 * setKey().
 */
class Chip8 : public Console {
 public:
  /// Construct a Chip8 instance with an empty memory and the font loaded.
  Chip8();

  ~Chip8() override = default;

  bool loadProgram(const uint8_t* data, std::size_t size) override;
  void reset() override;
  void stepFrame() override;
  bool isHalted() const override { return halted; }
  void setKey(int keyIndex, bool pressed) override;

  int width() const override { return highResolutionMode ? 128 : 64; }
  int height() const override { return highResolutionMode ? 64 : 32; }
  const uint8_t* framebuffer() const override { return framebuffer_.data(); }

  void setAudioSampleRate(int hz) override;
  std::size_t audioFrameCount() const override { return audio_.size() / 2; }
  const float* audioSamples() const override { return audio_.data(); }

 private:
  /// CPU instructions executed per 60 Hz frame (~720 instructions/second).
  static constexpr int kCyclesPerFrame = 12;
  /// Minimum beep length, in frames (6 frames = 100 ms).
  static constexpr int kBeepHoldFrames = 6;
  static constexpr double kToneHz = 440.0;
  static constexpr float kVolume = 0.09f;
  static constexpr double kRampSeconds = 0.003;  ///< anti-click fade

  /// Reset registers, memory (font only), display and timers.
  void resetState();

  /// Decrement the delay and sound timers (called once per frame, 60 Hz).
  void updateTimers();

  /// Convert the gfx buffer to the RGBA framebuffer.
  void renderFramebuffer();

  /// Synthesize this frame's audio samples into audio_.
  void generateAudio();

  /// Decode and execute a single fetched opcode.
  void executeOpcode(uint16_t opcode);

  /// Fetch, decode, and execute one instruction.
  void cycle();

  /**
   * CHIP-8 memory map:
   *  - 0x000-0x1FF: interpreter area (font set stored here).
   *  - 0x050-0x0A0: built-in font set.
   *  - 0x200-0xFFF: program ROM and work RAM.
   */
  uint8_t memory[4096];

  uint8_t V[16];  ///< General-purpose registers (V0 to VF).
  uint16_t I;     ///< Index register.
  uint16_t pc;    ///< Program counter.

  uint8_t gfx[128 * 64];  ///< Display buffer (128x64 for high-res mode).
  uint8_t draw_flag;      ///< Set when the framebuffer must be regenerated.

  uint8_t delay_timer;  ///< Delay timer, counts down at 60 Hz.
  uint8_t sound_timer;  ///< Sound timer; beeps while non-zero.

  uint16_t stack[16];  ///< Call stack.
  uint16_t sp;         ///< Stack pointer.

  uint8_t key[16];  ///< Current state of the hex keypad (1 = pressed).
  uint8_t rpl[16];  ///< RPL user flags (SCHIP persistent registers).

  /// Last key pressed since the previous frame (-1 if none); used by FX0A.
  int lastPressedKey = -1;

  bool halted;              ///< True when the program ended or crashed.
  bool highResolutionMode;  ///< True when SCHIP 128x64 mode is active.

  std::vector<uint8_t> rom_;  ///< Copy of the loaded ROM, used by reset().

  /// RGBA output, sized for the largest resolution.
  std::array<uint8_t, 128 * 64 * 4> framebuffer_{};

  // --- Audio state ---
  double sampleRate_ = 48000.0;
  double sampleAcc_ = 0.0;  ///< Fractional samples carried between frames.
  double phase_ = 0.0;      ///< Phase of the square wave, in [0, 1).
  float gain_ = 0.0f;       ///< Current fade gain, in [0, 1].
  int beepHoldFrames_ = 0;  ///< Remaining frames of the minimum beep length.
  bool beepOn_ = false;
  std::vector<float> audio_;  ///< Interleaved stereo samples of last frame.
};
