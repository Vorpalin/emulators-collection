#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "emulators/bus/master_system_bus/MasterSystemBus.hh"
#include "emulators/console/Console.hh"

/**
 * @file MasterSystem.hh
 * @brief Sega Master System console (and the base of the Game Gear).
 */

/**
 * @class MasterSystem
 * @brief Console front-end around MasterSystemBus: frame stepping, RGBA
 *        output, stereo audio, battery RAM and save states.
 *
 * Key indices (same layout as the Game Boy and NES so front-ends can share
 * their mappings): 0 Right, 1 Left, 2 Up, 3 Down, 4 A, 5 B, 6 Select,
 * 7 Start.
 *
 * | Key    | Master System          | Game Gear |
 * | ------ | ---------------------- | --------- |
 * | A      | Button 1               | Button 1  |
 * | B      | Button 2               | Button 2  |
 * | Start  | Pause (NMI)            | Start     |
 * | Select | unused                 | unused    |
 *
 * Only controller 1 is exposed through setKey().
 *
 * The picture size follows the VDP: 256x192 (256x224 / 256x240 when a game
 * selects the taller modes) on the Master System, 160x144 on the Game Gear.
 * Timing is NTSC (60 Hz) unless setPAL(true) is called.
 */
class MasterSystem : public Console {
 public:
  static constexpr int kMaxWidth = SegaVDP::kWidth;
  static constexpr int kMaxHeight = SegaVDP::kMaxHeight;

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

  explicit MasterSystem(SegaVDP::Model model = SegaVDP::Model::MasterSystem);
  ~MasterSystem() override = default;

  bool loadProgram(const uint8_t* data, std::size_t size) override;
  void reset() override;
  void stepFrame() override;
  bool isHalted() const override { return false; }
  void setKey(int key, bool pressed) override;

  int width() const override { return bus_.vdp().viewportWidth(); }
  int height() const override { return bus_.vdp().viewportHeight(); }
  const uint8_t* framebuffer() const override { return framebuffer_.data(); }

  void setAudioSampleRate(int hz) override;
  std::size_t audioFrameCount() const override { return audio_.size() / 2; }
  const float* audioSamples() const override { return audio_.data(); }

  std::string saveState() const override;
  bool loadState(const std::string& json) override;

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

  /** @brief Selects 50 Hz (PAL) or 60 Hz (NTSC) timing. */
  void setPAL(bool pal) { bus_.setPAL(pal); }
  /** @brief Reason of the last failed loadProgram(). */
  const std::string& error() const { return bus_.error(); }

 private:
  void renderFrame();

  MasterSystemBus bus_;
  bool loaded_ = false;
  int sampleRate_ = 48000;
  /// RGBA, laid out with the *current* width() as the row pitch.
  std::array<uint8_t, kMaxWidth * kMaxHeight * 4> framebuffer_{};
  std::vector<float> audio_;  ///< Interleaved stereo samples of last frame.
};
