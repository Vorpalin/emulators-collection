#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "emulators/audio/TIA1A/TIAAudio.hh"
#include "emulators/bus/atari2600_bus/Atari2600Bus.hh"
#include "emulators/console/Console.hh"

/**
 * @file Atari2600.hh
 * @brief Top-level Atari 2600 emulator, implementing the Console interface
 *        by driving the system bus, converting the TIA frame buffer to RGBA
 *        and synthesizing the TIA audio, without any platform dependency.
 */

struct Atari2600State {
  Atari2600BusState bus;

  std::array<bool, 7> keys{};

  double sampleRate;
  double sampleAcc;

  // État nécessaire uniquement à la continuité de l'audio.
  TIAAudioState audio;
};

/**
 * @class Atari2600
 * @brief Emulates an Atari 2600 console: owns the system bus (CPU, RIOT,
 *        TIA, cartridge) and the TIA audio engine.
 */
class Atari2600 : public Console {
 public:
  /// Key indices accepted by setKey().
  enum Key : int {
    kRight = 0,  ///< Joystick right.
    kLeft = 1,   ///< Joystick left.
    kDown = 2,   ///< Joystick down.
    kUp = 3,     ///< Joystick up.
    kFire = 4,   ///< Joystick fire button (player 0).
    kReset = 5,  ///< Console RESET switch.
    kSelect = 6  ///< Console SELECT switch.
  };

  /**
   * @brief Construct the emulator and wire up the audio write hook between
   *        the bus and the TIAAudio engine.
   */
  Atari2600();

  ~Atari2600() override = default;

  // The audio hook captures `this`: the object must not be copied or moved.
  Atari2600(const Atari2600&) = delete;
  Atari2600& operator=(const Atari2600&) = delete;

  bool loadProgram(const uint8_t* data, std::size_t size) override;
  void reset() override;
  void stepFrame() override;
  bool isHalted() const override { return false; }
  void setKey(int key, bool pressed) override;

  int width() const override { return TIA1A::kWidth; }
  int height() const override { return TIA1A::kHeight; }
  const uint8_t* framebuffer() const override { return framebuffer_.data(); }

  void setAudioSampleRate(int hz) override;
  std::size_t audioFrameCount() const override { return audio_.size() / 2; }
  const float* audioSamples() const override { return audio_.data(); }

  std::string saveState() const override;
  bool loadState(const std::string& filename) override;

 private:
  Atari2600State getState() const;
  void setState(const Atari2600State& state);

  /// Safety cap on bus ticks per frame (a normal frame needs ~7000).
  static constexpr int kMaxTicksPerFrame = 50000;
  /// NTSC frame rate used to size each frame's audio, as in the SDL version.
  static constexpr double kFrameRate = 59.92;

  /// Push the current joystick/switch state to the bus.
  void updateInput();

  /// Convert the TIA color-index frame to the RGBA framebuffer.
  void convertFrame();

  /// Synthesize this frame's audio into audio_ (interleaved stereo).
  void generateAudio();

  Atari2600Bus bus;  ///< System bus: CPU, cartridge, RIOT and TIA.
  TIAAudio audio;    ///< TIA audio engine, fed via the bus's audio hook.

  bool keys_[7] = {};  ///< Pressed state, indexed by Key.

  std::vector<uint8_t> framebuffer_;  ///< RGBA, width() * height() * 4 bytes.

  double sampleRate_ = 48000.0;
  double sampleAcc_ = 0.0;    ///< Fractional samples carried between frames.
  std::vector<float> mono_;   ///< Scratch buffer for TIAAudio::generate().
  std::vector<float> audio_;  ///< Interleaved stereo samples of last frame.
};
