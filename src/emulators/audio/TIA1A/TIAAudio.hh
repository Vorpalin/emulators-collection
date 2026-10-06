#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

/**
 * @file TIAAudio.hh
 * @brief Minimal TIA sound emulation (2 channels), platform independent.
 *
 * Feed it every CPU write to the TIA audio registers with write():
 *   - 0x15 AUDC0, 0x16 AUDC1 (waveform / noise type, 4 bits)
 *   - 0x17 AUDF0, 0x18 AUDF1 (frequency divider, 5 bits)
 *   - 0x19 AUDV0, 0x1A AUDV1 (volume, 4 bits)
 *
 * The host then pulls samples with generate(): the waveform is synthesized
 * from the latest register values. There is no audio device, no thread and
 * no SDL dependency here.
 */

/**
 * @brief State of the TIA audio engine, for serialization/debugging.
 */
struct TIAAudioState {
  uint8_t regs[6] = {};  ///< AUDC0, AUDC1, AUDF0, AUDF1, AUDV0, AUDV1.
  int div[2] = {};       ///< AUDF divider counters.
  int pos31[2] = {};     ///< Position in the 31-step pattern.
  int pos6[2] = {};      ///< Position in the 6-step pattern.
  int sub[2] = {};       ///< Extra /3 stage for modes 14 and 15.
  uint8_t p4[2] = {};    ///< 4-bit LFSR state.
  uint8_t p5[2] = {};    ///< 5-bit LFSR state.
  uint16_t p9[2] = {};   ///< 9-bit LFSR state (white noise).
  bool out[2] = {};      ///< Current output level of each channel.
};

/**
 * @brief JSON serialization for TIAAudioState.
 * @param j The JSON object to populate.
 * @param state The TIAAudioState to serialize.
 */
void to_json(nlohmann::json& j, const TIAAudioState& state);
/**
 * @brief JSON deserialization for TIAAudioState.
 * @param j The JSON object to read from.
 * @param state The TIAAudioState to populate.
 */
void from_json(const nlohmann::json& j, TIAAudioState& state);

/**
 * @class TIAAudio
 * @brief Generates the Atari 2600's two TIA audio channels as mono float
 *        samples, driven purely by the latest register values written from
 *        the emulated CPU.
 */
class TIAAudio {
 public:
  /// NTSC: 3.579545 MHz colour clock / 114 = ~31.4 kHz audio clock.
  static constexpr double kTiaAudioHz = 3579545.0 / 114.0;
  /// Peak amplitude in [0, 1] (8000 / 32767: kept quiet, as before).
  static constexpr float kAmplitude = 8000.0f / 32768.0f;

  /// Construct the audio engine with all registers and state zeroed.
  TIAAudio() { reset(); }

  /// Silence both channels and reset all generator state.
  void reset();

  /**
   * @brief Set the output sample rate used by generate().
   * @param hz Sample rate in Hz (ignored if <= 0).
   */
  void setSampleRate(int hz);

  /**
   * @brief Handle a CPU write to a TIA audio register.
   *
   * Ignores addresses outside the audio register range.
   *
   * @param addr  Register address; only the low 6 bits are used, and only
   *              0x15..0x1A are audio registers.
   * @param value Byte value written; masked to the register's valid bits.
   */
  void write(uint16_t addr, uint8_t value);

  /**
   * @brief Synthesize mono samples from the current register state.
   * @param out   Destination buffer for @p count float samples in [-1, 1].
   * @param count Number of samples to generate.
   */
  void generate(float* out, int count);

  /**
   * @brief Returns the current TIA audio state for serialization/debugging.
   * @return Current state of both channels and their generator state.
   */
  void setState(const TIAAudioState& state);

  /**
   * @brief Sets the current TIA audio state from a serialized representation.
   * @param state The TIA audio state to restore.
   */
  TIAAudioState getState() const;

 private:
  /// @brief Per-channel waveform generator state.
  struct Channel {
    int div = 0;          ///< AUDF divider counter.
    int pos31 = 0;        ///< Position in the 31-step pattern.
    int pos6 = 0;         ///< Position in the 6-step pattern.
    int sub = 0;          ///< Extra /3 stage for modes 14 and 15.
    uint8_t p4 = 0x0F;    ///< 4-bit LFSR state.
    uint8_t p5 = 0x1F;    ///< 5-bit LFSR state.
    uint16_t p9 = 0x1FF;  ///< 9-bit LFSR state (white noise).
    bool out = false;     ///< Current output level of this channel.
  };

  /// Clock a 4-bit maximal-length LFSR (x^4+x^3+1); returns the new bit.
  bool lfsr4(uint8_t& r);
  /// Clock a 5-bit maximal-length LFSR (x^5+x^3+1); returns the new bit.
  bool lfsr5(uint8_t& r);
  /// Clock a 9-bit maximal-length LFSR (x^9+x^5+1); returns the new bit.
  bool lfsr9(uint16_t& r);
  /// Duty pattern for the div-31 square wave modes (18 high, 13 low).
  bool pattern31(int pos);

  /**
   * @brief Advance one channel's waveform generator by one tick of the
   *        ~31.4 kHz TIA audio clock, per the AUDC waveform selection.
   */
  void tick(Channel& ch, uint8_t audc, uint8_t audf);

  double sampleRate_ = 44100.0;  ///< Active sample rate, in Hz.

  /// Register values: 0 AUDC0, 1 AUDC1, 2 AUDF0, 3 AUDF1, 4 AUDV0, 5 AUDV1.
  uint8_t regs_[6] = {};

  Channel ch_[2];          ///< Waveform generator state per channel.
  double clockAcc_ = 0.0;  ///< Fractional TIA-clock accumulator.
  float xPrev_ = 0.0f, yPrev_ = 0.0f;  ///< High-pass filter state.
};
