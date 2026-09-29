#pragma once

/**
 * @file TIAAudio.hh
 * @brief Minimal TIA sound emulation (2 channels) with SDL2 output,
 *        header-only.
 *
 * Feed it every CPU write to the TIA audio registers with write():
 *   - 0x15 AUDC0, 0x16 AUDC1 (waveform / noise type, 4 bits)
 *   - 0x17 AUDF0, 0x18 AUDF1 (frequency divider, 5 bits)
 *   - 0x19 AUDV0, 0x1A AUDV1 (volume, 4 bits)
 *
 * The waveform is generated in the SDL audio thread from the latest
 * register values, so the emulator never has to queue samples.
 */

#include <SDL2/SDL.h>

#include <atomic>
#include <cstdint>
#include <iostream>

/**
 * @class TIAAudio
 * @brief Generates the Atari 2600's two TIA audio channels in real time
 *        via an SDL2 audio callback, driven purely by the latest register
 *        values written from the emulated CPU.
 */
class TIAAudio {
 public:
  /// NTSC: 3.579545 MHz colour clock / 114 = ~31.4 kHz audio clock.
  static constexpr double kTiaAudioHz = 3579545.0 / 114.0;
  static constexpr float kAmplitude =
      8000.0f;  ///< Peak amplitude, out of 32767 (kept quiet).

  /**
   * @brief Construct the audio engine (does not open the audio device;
   *        call open() for that) and zero all registers.
   */
  TIAAudio();

  /**
   * @brief Destroy the audio engine, closing the audio device if open.
   */
  ~TIAAudio() { close(); }
  TIAAudio(const TIAAudio&) = delete;
  TIAAudio& operator=(const TIAAudio&) = delete;

  /**
   * @brief Initialize SDL audio and open a playback device.
   *
   * The device starts corked (paused); playback begins automatically on
   * the first write() that sets a non-zero volume.
   *
   * @param freq          Desired sample rate, in Hz.
   * @param bufferSamples Desired SDL audio buffer size, in samples.
   * @return True on success, false if SDL audio init or device open failed.
   */
  bool open(int freq = 44100, int bufferSamples = 512);

  /**
   * @brief Close the audio device and shut down the SDL audio subsystem
   *        if this instance initialized it.
   *
   * Safe to call several times, and safe after SDL_Quit(). Call it while
   * SDL is still alive (e.g. at the end of run()) rather than relying on
   * the destructor, which may run after the program has already shut SDL
   * down.
   */
  void close();

  /**
   * @brief Handle a CPU write to a TIA audio register.
   *
   * Ignores addresses outside the audio register range. Automatically
   * un-pauses the audio device the moment a channel is given a non-zero
   * volume.
   *
   * @param addr  Register address; only the low 6 bits are used, and only
   *              0x15..0x1A are audio registers.
   * @param value Byte value written; masked to the register's valid bits.
   */
  void write(uint16_t addr, uint8_t value);

  /**
   * @brief Per-frame housekeeping: corks the audio stream after ~0.5 s of
   *        silence (both channel volumes at zero), which also flushes any
   *        latency the audio server has accumulated. Call once per
   *        emulated frame.
   */
  void update();

  /**
   * @brief Override the sample rate used for waveform generation without
   *        reopening the device (e.g. after SDL reports a different rate).
   * @param hz New sample rate, in Hz.
   */
  void setSampleRate(int hz);

  /**
   * @brief Synthesize audio samples from the current register state.
   *
   * Called from the SDL audio thread via the SDL callback; not intended
   * to be called directly by emulator code.
   *
   * @param out   Destination buffer for @p count mono 16-bit samples.
   * @param count Number of samples to generate.
   */
  void generate(int16_t* out, int count);

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

  /**
   * @brief Clock a 4-bit maximal-length LFSR (x^4+x^3+1).
   * @param r In/out: register state.
   * @return New output bit.
   */
  bool lfsr4(uint8_t& r);
  /**
   * @brief Clock a 5-bit maximal-length LFSR (x^5+x^3+1).
   * @param r In/out: register state.
   * @return New output bit.
   */
  bool lfsr5(uint8_t& r);
  /**
   * @brief Clock a 9-bit maximal-length LFSR (x^9+x^5+1), used for white noise.
   * @param r In/out: register state.
   * @return New output bit.
   */
  bool lfsr9(uint16_t& r);
  /**
   * @brief Duty pattern for the div-31 square wave modes (18 high, 13 low).
   * @param pos Position within the 31-step cycle.
   * @return True while the pattern is "high".
   */
  bool pattern31(int pos);

  /**
   * @brief Advance one channel's waveform generator by one tick of the
   *        ~31.4 kHz TIA audio clock, per the AUDC waveform selection.
   * @param ch   Channel state to update.
   * @param audc AUDCx register value (waveform/noise type).
   * @param audf AUDFx register value (frequency divider).
   */
  void tick(Channel& ch, uint8_t audc, uint8_t audf);

  /**
   * @brief SDL audio callback trampoline; forwards to generate().
   * @param userdata Pointer to the owning TIAAudio instance.
   * @param stream   Destination buffer, as raw bytes.
   * @param len      Length of @p stream, in bytes.
   */
  static void callback(void* userdata, Uint8* stream, int len);

  SDL_AudioDeviceID dev_ = 0;  ///< SDL audio device handle (0 if not open).
  bool audioInit_ = false;     ///< True if this instance called
                               ///< SDL_InitSubSystem(SDL_INIT_AUDIO).
  bool paused_ = true;         ///< True while the device is corked/paused.
  int idleFrames_ = 0;  ///< Consecutive silent update() calls, for auto-pause.
  double sampleRate_ = 44100.0;  ///< Active sample rate, in Hz.

  /// Register values: 0 AUDC0, 1 AUDC1, 2 AUDF0, 3 AUDF1, 4 AUDV0, 5 AUDV1.
  std::atomic<uint8_t> regs_[6];

  // Audio-thread state only.
  Channel ch_[2];          ///< Waveform generator state per channel.
  double clockAcc_ = 0.0;  ///< Fractional TIA-clock accumulator.
  float xPrev_ = 0.0f, yPrev_ = 0.0f;  ///< High-pass filter state.
};
