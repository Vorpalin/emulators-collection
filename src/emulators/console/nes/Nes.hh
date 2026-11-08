#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "emulators/bus/nes_bus/NesBus.hh"
#include "emulators/console/Console.hh"

/**
 * @file Nes.hh
 * @brief Nintendo Entertainment System (NTSC) console.
 */

/**
 * @class Nes
 * @brief Console front-end around NesBus: frame stepping, RGBA output,
 *        stereo audio and save states.
 *
 * Key indices (same layout as the Game Boy so front-ends can share their
 * mappings): 0 Right, 1 Left, 2 Up, 3 Down, 4 A, 5 B, 6 Select, 7 Start.
 */
class Nes : public Console {
 public:
  static constexpr int kWidth = PPU2C02::kWidth;
  static constexpr int kHeight = PPU2C02::kHeight;

  enum Key : int {
    kRight = 0,
    kLeft = 1,
    kUp = 2,
    kDown = 3,
    kA = 4,
    kB = 5,
    kSelect = 6,
    kStart = 7,
  };

  Nes();
  ~Nes() override = default;

  bool loadProgram(const uint8_t* data, std::size_t size) override;
  void reset() override;
  void stepFrame() override;
  bool isHalted() const override { return bus_.isHalted(); }
  void setKey(int key, bool pressed) override;

  bool hasBattery() const override {
    return loaded_ && bus_.cartridge().hasBattery();
  }
  std::vector<uint8_t> saveRam() const override {
    return bus_.cartridge().batteryRam();
  }
  bool loadRam(const uint8_t* data, std::size_t size) override {
    return loaded_ && bus_.cartridge().loadBatteryRam(data, size);
  }
  bool consumeRamDirty() override {
    return loaded_ && bus_.cartridge().consumeRamDirty();
  }

  int width() const override { return kWidth; }
  int height() const override { return kHeight; }
  const uint8_t* framebuffer() const override { return framebuffer_.data(); }

  void setAudioSampleRate(int hz) override;
  std::size_t audioFrameCount() const override { return audio_.size() / 2; }
  const float* audioSamples() const override { return audio_.data(); }

  std::string saveState() const override;
  bool loadState(const std::string& json) override;

  /** @brief Reason of the last failed loadProgram(). */
  const std::string& error() const { return bus_.error(); }

 private:
  void renderFrame();

  NesBus bus_;
  bool loaded_ = false;
  std::array<uint8_t, kWidth * kHeight * 4> framebuffer_{};  ///< RGBA.
  std::vector<float> audio_;  ///< Interleaved stereo samples of last frame.
  std::array<uint32_t, 512> palette_{};  ///< 64 colours x 8 emphasis modes.
};
