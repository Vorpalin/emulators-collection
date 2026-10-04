#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "emulators/bus/game_boy_bus/GameBoyBus.hh"
#include "emulators/console/Console.hh"

/**
 * @file GameBoy.hh
 * @brief Top-level Game Boy emulator, platform independent.
 */

struct GameBoyState {
  GameBoyBusState bus;

  double cyclesPerSample;
  double phase;
  double sumL;
  double sumR;
  double weight;
};

/**
 * @class GameBoy
 * @brief Top-level Game Boy system.
 *
 * Owns the GameBoyBus (which in turn owns the CPU, PPU, cartridge, timer,
 * joypad, APU...) and implements the Console interface. It has no SDL
 * dependency: video is exposed as an RGBA framebuffer, audio as float PCM
 * samples, and input goes through setKey().
 */
class GameBoy : public Console {
 public:
  /// Key indices accepted by setKey() (same values as GameBoyController).
  enum Key : int {
    kRight = 0,
    kLeft = 1,
    kUp = 2,
    kDown = 3,
    kA = 4,
    kB = 5,
    kSelect = 6,
    kStart = 7
  };

  /** @brief Constructs the emulator and its bus. */
  GameBoy();
  ~GameBoy() override = default;

  bool loadProgram(const uint8_t* data, std::size_t size) override;
  void reset() override;
  void stepFrame() override;
  bool isHalted() const override { return false; }
  void setKey(int key, bool pressed) override;

  int width() const override { return kWidth; }
  int height() const override { return kHeight; }
  const uint8_t* framebuffer() const override { return framebuffer_.data(); }

  void setAudioSampleRate(int hz) override;
  std::size_t audioFrameCount() const override { return audio_.size() / 2; }
  const float* audioSamples() const override { return audio_.data(); }

  /** @brief Gives access to the system bus (useful for debugging/tests). */
  GameBoyBus& getBus() { return bus; }

  GameBoyState getState() const;
  void setState(const GameBoyState& state);

  void saveState(const std::string& filename) const override;
  void loadState(const std::string& filename) override;

 private:
  static constexpr int kWidth = 160;
  static constexpr int kHeight = 144;
  static constexpr double kCpuHz = 4194304.0;  ///< T-cycles per second.
  /// Safety cap: if the LCD is off no frame is ever signalled, so give up
  /// after two frames' worth of T-cycles instead of looping forever.
  static constexpr uint32_t kMaxCyclesPerStep = 2 * 70224;

  /// Convert the PPU's 2-bit shades to the RGBA framebuffer.
  void convertFrame();

  GameBoyBus bus;  ///< System bus, owner of all hardware components.

  std::array<uint8_t, kWidth * kHeight * 4> framebuffer_{};  ///< RGBA output.

  // --- Audio: APU is sampled every instruction, then box-filtered ---
  double cyclesPerSample_ = kCpuHz / 48000.0;
  double phase_ = 0.0;  ///< T-cycles since the last output sample.
  double sumL_ = 0.0;   ///< Cycle-weighted accumulators for the filter.
  double sumR_ = 0.0;
  double weight_ = 0.0;
  std::vector<float> audio_;  ///< Interleaved stereo samples of last frame.
};
