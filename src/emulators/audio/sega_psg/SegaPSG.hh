#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <vector>

/**
 * @file SegaPSG.hh
 * @brief SN76489-compatible programmable sound generator of the Sega Master
 *        System and Game Gear: three square-wave tone channels and one noise
 *        channel.
 */

/** @brief Serializable state of the PSG. */
struct SegaPSGState {
  std::array<uint16_t, 3> tonePeriod{};         ///< 10-bit tone periods.
  std::array<int, 4> counter{};                 ///< Down counters (3 tones + noise).
  std::array<bool, 4> flip{};                   ///< Output flip-flops.
  std::array<uint8_t, 4> volume{{15, 15, 15, 15}};  ///< Attenuation, 15 = off.
  uint8_t noiseControl = 0;                     ///< Bit 2 white noise, bits 1-0 rate.
  uint16_t lfsr = 0x8000;                       ///< Noise shift register.
  uint8_t latch = 0;                            ///< Latched register (bits 6-4 of the byte).
  uint8_t stereo = 0xFF;                        ///< Game Gear stereo mask.
  int cycleRemainder = 0;                       ///< CPU cycles not yet ticked (0-15).
  double phase = 0.0;                           ///< Ticks since the last output sample.
  double accL = 0.0, accR = 0.0, accCount = 0.0;
  double hpPrevInL = 0.0, hpPrevOutL = 0.0;
  double hpPrevInR = 0.0, hpPrevOutR = 0.0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SegaPSGState, tonePeriod, counter, flip,
                                   volume, noiseControl, lfsr, latch, stereo,
                                   cycleRemainder, phase, accL, accR, accCount,
                                   hpPrevInL, hpPrevOutL, hpPrevInR,
                                   hpPrevOutR)

/**
 * @class SegaPSG
 * @brief Cycle-driven PSG producing interleaved stereo float samples.
 *
 * The chip ticks once every 16 CPU cycles (223,721 Hz on NTSC). A tone
 * channel toggles its output each time its counter reaches zero, giving a
 * frequency of clock / (32 * period). Channel 3 is either periodic or white
 * noise (16-bit LFSR, taps on bits 0 and 3), clocked at one of three fixed
 * rates or at the rate of tone channel 2. A tone period of 0 or 1 holds the
 * output high (the "sample playback" trick used by some games, which then
 * modulate the volume directly).
 *
 * Programming (single data port, 0x7F on the SMS):
 *  - byte with bit 7 set: `1 cc t dddd` latches channel `cc` (0-2 tone,
 *    3 noise), `t` = 1 for the attenuation register, 0 for tone/noise, and
 *    writes the low 4 bits `dddd`;
 *  - byte with bit 7 clear: `0 - dddddd` writes the latched register (the
 *    high 6 bits of a tone period, or the low bits of the others).
 *
 * The Game Gear stereo register (port 0x06) is supported through
 * writeStereo(); on the Master System all channels stay on both sides.
 */
class SegaPSG {
 public:
  static constexpr double kClockHz = 3579545.0;  ///< NTSC CPU / PSG clock.
  static constexpr int kCyclesPerTick = 16;      ///< CPU cycles per PSG tick.

  SegaPSG();

  /** @brief Resets the chip (all channels silent, noise register reset). */
  void reset();

  /** @brief Sets the output sample rate in Hz (default 48000). */
  void setSampleRate(int hz);

  /** @brief Selects the 50 Hz clock (PAL: 3,546,893 Hz). */
  void setPAL(bool pal) { clockHz_ = pal ? 3546893.0 : kClockHz; updateRate(); }

  /** @brief Enables or disables the DC-blocking high-pass filter (on by default). */
  void setHighPass(bool enabled) { highPass_ = enabled; }

  /** @brief Advances the chip by @p cpuCycles Z80 cycles. */
  void step(uint32_t cpuCycles);

  /** @brief Writes a byte to the PSG data port. */
  void write(uint8_t value);

  /** @brief Writes the Game Gear stereo register (bits 7-4 left, 3-0 right). */
  void writeStereo(uint8_t value) { state_.stereo = value; }

  /** @brief Interleaved stereo samples (L, R, L, R...) since clearSamples(). */
  const std::vector<float> &samples() const { return samples_; }
  /** @brief Number of stereo frames available. */
  size_t frameCount() const { return samples_.size() / 2; }
  /** @brief Discards the samples produced so far. */
  void clearSamples() { samples_.clear(); }

  SegaPSGState getState() const { return state_; }
  void setState(const SegaPSGState &s) { state_ = s; }

 private:
  SegaPSGState state_;
  std::vector<float> samples_;
  int sampleRate_ = 48000;
  double clockHz_ = kClockHz;
  double ticksPerSample_ = 0.0;
  double hpAlpha_ = 0.0;
  bool highPass_ = true;
  std::array<float, 16> amplitude_{};

  void updateRate();
  void tick();
  void shiftNoise();
  bool channelOutput(int ch) const;
  void emit(double l, double r);
};
