/**
 * @file APU.hh
 * @brief Game Boy Audio Processing Unit (APU) emulation.
 *
 * The Game Boy APU has four sound channels, each with its own set of
 * NRxx registers mapped at 0xFF10-0xFF3F:
 *  - Channel 1: square wave with frequency sweep (NR10-NR14, 0xFF10-0xFF14)
 *  - Channel 2: square wave without sweep (NR21-NR24, 0xFF16-0xFF19)
 *  - Channel 3: user-defined 4-bit wave (NR30-NR34, 0xFF1A-0xFF1E, plus the
 *    32-sample wave RAM at 0xFF30-0xFF3F)
 *  - Channel 4: pseudo-random noise via an LFSR (NR41-NR44, 0xFF20-0xFF23)
 *
 * All four channels are mixed and routed to the left/right output through
 * NR50 (0xFF24, master volume/VIN panning) and NR51 (0xFF25, per-channel
 * stereo panning). NR52 (0xFF26) holds the master audio enable bit and the
 * per-channel "still playing" status flags.
 *
 * A 512 Hz frame sequencer (clocked by stepFrame(), typically driven from
 * DIV-APU) advances the length counters, the volume envelopes and the
 * frequency sweep at their respective standard rates.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

/**
 * @brief State of sound Channel 1 (square wave with frequency sweep).
 *
 * Channel 1 is a square/pulse wave generator identical to Channel 2, with
 * the addition of a frequency sweep unit (NR10) that can periodically
 * raise or lower its frequency, and optionally disable the channel if the
 * frequency over/underflows.
 */
struct Channel1 {
  /// Whether the channel is currently active (audible / contributing to
  /// the NR52 status bit). Cleared when the length counter expires, when
  /// the volume envelope decays to silence, or when the sweep overflows.
  bool enabled = false;

  /// NR10 (0xFF10) - Sweep period, direction and shift amount.
  uint8_t nr10 = 0;
  /// NR11 (0xFF11) - Wave duty pattern (bits 6-7) and initial length
  /// timer load value (bits 0-5).
  uint8_t nr11 = 0;
  /// NR12 (0xFF12) - Initial envelope volume, direction and sweep pace.
  uint8_t nr12 = 0;
  /// NR13 (0xFF13) - Lower 8 bits of the 11-bit period/frequency value.
  uint8_t nr13 = 0;
  /// NR14 (0xFF14) - Trigger bit, length-enable bit, and upper 3 bits of
  /// the 11-bit period/frequency value.
  uint8_t nr14 = 0;

  /// Frequency timer, in APU clock ticks, counting down to the next
  /// waveform step (duty cycle advance).
  int timer = 0;
  /// Current step (0-7) within the selected duty cycle waveform.
  int duty = 0;

  /// Length counter, decremented by stepLength(); the channel is
  /// disabled when it reaches zero and the length counter is enabled.
  int length = 0;
  /// Current output volume (0-15) as driven by the volume envelope.
  int volume = 0;
  /// Countdown, in envelope steps, until the next volume envelope
  /// increment/decrement performed by stepVolumeEnvelope().
  int envelopeTimer = 0;

  /// Countdown, in sweep steps, until the next frequency update
  /// performed by stepSweep().
  int sweepTimer = 0;
  /// Working copy of the channel's frequency used by the sweep unit,
  /// separate from the NR13/NR14 value so sweep calculations don't
  /// disturb the registers directly.
  int shadowFrequency = 0;
  /// Whether the frequency sweep unit is currently active for this
  /// channel (set on trigger based on the sweep period/shift settings).
  bool sweepEnabled = false;

  /// Last computed analog sample (DAC input, 0-15) for this channel,
  /// before panning/mixing.
  uint8_t output = 0;
};

/**
 * @brief State of sound Channel 2 (square wave, no sweep).
 *
 * Functionally identical to Channel 1 minus the frequency sweep unit.
 */
struct Channel2 {
  /// Whether the channel is currently active (audible / contributing to
  /// the NR52 status bit).
  bool enabled = false;

  /// NR21 (0xFF16) - Wave duty pattern (bits 6-7) and initial length
  /// timer load value (bits 0-5).
  uint8_t nr21 = 0;
  /// NR22 (0xFF17) - Initial envelope volume, direction and sweep pace.
  uint8_t nr22 = 0;
  /// NR23 (0xFF18) - Lower 8 bits of the 11-bit period/frequency value.
  uint8_t nr23 = 0;
  /// NR24 (0xFF19) - Trigger bit, length-enable bit, and upper 3 bits of
  /// the 11-bit period/frequency value.
  uint8_t nr24 = 0;

  /// Frequency timer, in APU clock ticks, counting down to the next
  /// waveform step (duty cycle advance).
  int timer = 0;
  /// Current step (0-7) within the selected duty cycle waveform.
  int duty = 0;

  /// Length counter, decremented by stepLength().
  int length = 0;
  /// Current output volume (0-15) as driven by the volume envelope.
  int volume = 0;
  /// Countdown, in envelope steps, until the next volume envelope
  /// increment/decrement performed by stepVolumeEnvelope().
  int envelopeTimer = 0;

  /// Last computed analog sample (DAC input, 0-15) for this channel,
  /// before panning/mixing.
  uint8_t output = 0;
};

/**
 * @brief State of sound Channel 3 (user-defined wave).
 *
 * Channel 3 plays back a 32-sample, 4-bit-per-sample waveform stored in
 * wave RAM (0xFF30-0xFF3F), looping continuously while enabled. It has a
 * length counter like the other channels but no volume envelope; output
 * level is instead a simple shift (mute/100%/50%/25%) selected by NR32.
 */
struct Channel3 {
  /// Whether the channel is currently active (audible / contributing to
  /// the NR52 status bit).
  bool enabled = false;

  /// NR30 (0xFF1A) - DAC power bit (bit 7); the channel's DAC, and so
  /// the channel itself, is silenced when this bit is clear.
  uint8_t nr30 = 0;
  /// NR31 (0xFF1B) - Initial length timer load value.
  uint8_t nr31 = 0;
  /// NR32 (0xFF1C) - Output level selector (mute, 100%, 50%, 25%).
  uint8_t nr32 = 0;
  /// NR33 (0xFF1D) - Lower 8 bits of the 11-bit period/frequency value.
  uint8_t nr33 = 0;
  /// NR34 (0xFF1E) - Trigger bit, length-enable bit, and upper 3 bits of
  /// the 11-bit period/frequency value.
  uint8_t nr34 = 0;

  /// Frequency timer, in APU clock ticks, counting down to the next
  /// wave sample advance.
  int timer = 0;
  /// Length counter, decremented by stepLength().
  int length = 0;
  /// Index (0-31) of the current sample within the wave table.
  int position = 0;

  /// 16-byte wave RAM (0xFF30-0xFF3F), holding 32 packed 4-bit samples
  /// (two samples per byte, high nibble first).
  std::array<uint8_t, 16> waveTable{};

  /// Last computed analog sample (DAC input, 0-15) for this channel,
  /// before panning/mixing.
  uint8_t output = 0;
};

/**
 * @brief State of sound Channel 4 (noise).
 *
 * Channel 4 generates pseudo-random noise by clocking a linear feedback
 * shift register (LFSR) at a frequency derived from NR43, with a volume
 * envelope identical in behavior to Channels 1 and 2.
 */
struct Channel4 {
  /// Whether the channel is currently active (audible / contributing to
  /// the NR52 status bit).
  bool enabled = false;

  /// NR41 (0xFF20) - Initial length timer load value.
  uint8_t nr41 = 0;
  /// NR42 (0xFF21) - Initial envelope volume, direction and sweep pace.
  uint8_t nr42 = 0;
  /// NR43 (0xFF22) - Clock shift, LFSR width (7/15-bit) and divisor code,
  /// together determining the noise frequency.
  uint8_t nr43 = 0;
  /// NR44 (0xFF23) - Trigger bit and length-enable bit.
  uint8_t nr44 = 0;

  /// Frequency timer, in APU clock ticks, counting down to the next
  /// LFSR shift.
  int timer = 0;
  /// Length counter, decremented by stepLength().
  int length = 0;
  /// Current output volume (0-15) as driven by the volume envelope.
  int volume = 0;
  /// Countdown, in envelope steps, until the next volume envelope
  /// increment/decrement performed by stepVolumeEnvelope().
  int envelopeTimer = 0;

  /// Linear feedback shift register state. Reset to all bits set
  /// (0x7FFF) on trigger; its lowest bit (inverted) drives the channel's
  /// output level at each timer expiry.
  uint16_t lfsr = 0x7FFF;

  /// Last computed analog sample (DAC input, 0-15) for this channel,
  /// before panning/mixing.
  uint8_t output = 0;
};

/**
 * @brief Complete state of the APU, including all four channels and the
 *        global mixing registers.
 */
struct APUState {
  Channel1 channel1;
  Channel2 channel2;
  Channel3 channel3;
  Channel4 channel4;

  bool audioEnabled;
  int frameStep;
  int frameTimer;

  uint8_t nr50;
  uint8_t nr51;
};

/// @name JSON serialization
/// @{
void to_json(nlohmann::json& j,
             const APUState& state);  ///< JSON serialization for APUState
void to_json(nlohmann::json& j,
             const Channel1& ch1);  ///< JSON serialization for Channel1
void to_json(nlohmann::json& j,
             const Channel2& ch2);  ///< JSON serialization for Channel2
void to_json(nlohmann::json& j,
             const Channel3& ch3);  ///< JSON serialization for Channel3
void to_json(nlohmann::json& j,
             const Channel4& ch4);  ///< JSON serialization for Channel4
/// @}

/// @name JSON deserialization
/// @{
void from_json(const nlohmann::json& j,
               APUState& state);  ///< JSON deserialization for APUState
void from_json(const nlohmann::json& j,
               Channel1& ch1);  ///< JSON deserialization for Channel1
void from_json(const nlohmann::json& j,
               Channel2& ch2);  ///< JSON deserialization for Channel2
void from_json(const nlohmann::json& j,
               Channel3& ch3);  ///< JSON deserialization for Channel3
void from_json(const nlohmann::json& j,
               Channel4& ch4);  ///< JSON deserialization for Channel4
/// @}

/**
 * @brief Game Boy Audio Processing Unit.
 *
 * Owns the state of the four sound channels and the global mixing
 * registers (NR50/NR51/NR52), advances them in lock-step with the CPU via
 * tick(), and exposes the current mixed audio sample(s) for playback.
 *
 * Typical usage from the bus/SoC glue code:
 * @code
 * apu.tick(cyclesElapsed);
 * if (apu.write(address, value)) { ... } // for addresses 0xFF10-0xFF3F
 * float mono = apu.getSample();
 * @endcode
 */
class APU {
 public:
  /// Constructs the APU with all channels silenced and registers at
  /// their post-reset values.
  APU();
  ~APU();

  /**
   * @brief Advances the APU state machine.
   * @param cycles Number of elapsed CPU clock cycles (T-cycles) to
   *        emulate since the previous call.
   *
   * Drives each channel's frequency timer (and therefore its waveform
   * generation / LFSR shifting), and clocks the 512 Hz frame sequencer
   * (stepFrame()) which in turn periodically invokes stepLength(),
   * stepVolumeEnvelope() and stepSweep() at their standard rates.
   */
  void tick(uint32_t cycles);

  /// Resets all channels, the frame sequencer and the global mixing
  /// registers (NR50/NR51/NR52) to their power-on state.
  void reset();

  /**
   * @brief Writes a value to an APU register or to wave RAM.
   * @param addr Absolute address in the range 0xFF10-0xFF3F.
   * @param data Byte value to write.
   *
   * Handles writes to the per-channel NRxx registers (including
   * triggering a channel when its trigger bit is set), to NR50/NR51/NR52,
   * and to Channel 3's wave RAM (0xFF30-0xFF3F). Writes to most audio
   * registers are ignored while the APU is powered off (NR52 bit 7
   * clear), per hardware behavior.
   */
  void write(uint16_t addr, uint8_t data);

  /**
   * @brief Reads the value of an APU register or wave RAM byte.
   * @param addr Absolute address in the range 0xFF10-0xFF3F.
   * @return The register's current value. Write-only bits read back as
   *         set (1), per hardware behavior.
   */
  uint8_t read(uint16_t addr);

  /**
   * @brief Returns the current mono-mixed audio sample.
   * @return Mixed sample of all enabled/audible channels, normalized to
   *         the [-1.0, 1.0] (or [0.0, 1.0], depending on the DAC
   *         convention used by the implementation) floating-point range.
   */
  float getSample();  ///< mono mix

  /**
   * @brief Returns the current stereo-mixed audio sample.
   * @param[out] left Mixed sample for the left output, panned and scaled
   *        according to NR50/NR51.
   * @param[out] right Mixed sample for the right output, panned and
   *        scaled according to NR50/NR51.
   */
  void getStereoSample(float& left, float& right);  ///< per-side mix

  /**
   * @brief Sets the current APU state from a serialized representation.
   * @param state The APU state to restore.
   */
  void setState(const APUState& state);

  /**
   * @brief Returns the current APU state for serialization/debugging.
   * @return Current state of all four channels and the global mixing
   *         registers.
   */
  APUState getState() const;

 private:
  /// Master audio power state (NR52 bit 7). When false, all channels are
  /// silenced and most register writes are ignored.
  bool audioEnabled = false;

  /// Current step (0-7) of the 512 Hz frame sequencer, advanced by
  /// stepFrame(); determines which of stepLength(), stepSweep() and
  /// stepVolumeEnvelope() fire on a given frame sequencer tick.
  int frameStep = 0;
  /// Countdown, in APU clock ticks, until the next frame sequencer step.
  int frameTimer = 0;

  /// Advances the frame sequencer by one step, dispatching to
  /// stepLength() (steps 0, 2, 4, 6), stepSweep() (steps 2, 6) and
  /// stepVolumeEnvelope() (step 7), matching real hardware timing.
  void stepFrame();
  /// Clocks the length counter of every channel that has its
  /// length-enable bit set, disabling any channel whose counter reaches
  /// zero.
  void stepLength();
  /// Clocks the volume envelope of Channels 1, 2 and 4, incrementing or
  /// decrementing their volume according to NRx2's envelope direction and
  /// pace.
  void stepVolumeEnvelope();
  /// Clocks Channel 1's frequency sweep unit, recalculating and applying
  /// its frequency and disabling the channel on overflow.
  void stepSweep();

  /// State of sound Channel 1 (square wave with sweep).
  struct Channel1 channel1;
  /// State of sound Channel 2 (square wave).
  struct Channel2 channel2;
  /// State of sound Channel 3 (user-defined wave).
  struct Channel3 channel3;
  /// State of sound Channel 4 (noise).
  struct Channel4 channel4;

  /// NR50 (0xFF24) - Master volume for left/right output and VIN
  /// (cartridge audio input) panning.
  uint8_t nr50 = 0;
  /// NR51 (0xFF25) - Per-channel left/right stereo panning.
  uint8_t nr51 = 0;

  /// Computes Channel 1's next frequency from its shadow frequency and
  /// NR10's sweep shift/direction, used by stepSweep() both to check for
  /// overflow and to obtain the value written back on an actual sweep
  /// update.
  /// @return The newly calculated 11-bit frequency value.
  int calculateSweepFrequency();
  /// Triggers (restarts) Channel 1: reloads its length counter if
  /// expired, resets the duty timer and envelope, loads the sweep's
  /// shadow frequency, and re-enables the channel (DAC permitting).
  void triggerChannel1();
  /// Triggers (restarts) Channel 2, analogous to triggerChannel1() minus
  /// the sweep setup.
  void triggerChannel2();
  /// Triggers (restarts) Channel 3: reloads its length counter if
  /// expired, resets the wave position, and re-enables the channel
  /// (DAC/NR30 permitting).
  void triggerChannel3();
  /// Triggers (restarts) Channel 4: reloads its length counter if
  /// expired, resets the envelope and the LFSR to 0x7FFF, and re-enables
  /// the channel (DAC permitting).
  void triggerChannel4();
};
