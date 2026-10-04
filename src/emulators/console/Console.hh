#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>

/**
 * @file Console.hh
 * @brief Platform-independent interface implemented by all supported system
 *        emulators (e.g. Chip8, Atari2600).
 *
 * The interface has no dependency on SDL (or any other platform library):
 * the emulator exposes its video and audio output as plain buffers and
 * receives input through setKey(). The host (SDL frontend, or a web page
 * through WebAssembly) is responsible for display, sound and timing.
 */

/**
 * @class Console
 * @brief Abstract base class defining the interface a concrete emulator must
 *        implement so it can be driven by any frontend.
 */
class Console {
 public:
  virtual ~Console() = default;

  /**
   * @brief Load a program/ROM from memory and reset the machine.
   * @param data Pointer to the ROM bytes.
   * @param size Number of bytes.
   * @return false if the ROM is invalid or too large.
   */
  virtual bool loadProgram(const uint8_t* data, std::size_t size) = 0;

  /**
   * @brief Reset the machine to its initial state (the ROM stays loaded).
   */
  virtual void reset() = 0;

  /**
   * @brief Run the emulation for exactly one video frame, then return.
   *        Never blocks. Also produces the audio for that frame.
   */
  virtual void stepFrame() = 0;

  /**
   * @brief True when the program ended (exit opcode, fatal error...).
   */
  virtual bool isHalted() const = 0;

  /**
   * @brief Update the state of a key/button.
   * @param key Emulator-specific key index.
   * @param pressed True when pressed, false when released.
   */
  virtual void setKey(int key, bool pressed) = 0;

  /// Current display width in pixels (may change at runtime).
  virtual int width() const = 0;

  /// Current display height in pixels (may change at runtime).
  virtual int height() const = 0;

  /**
   * @brief RGBA framebuffer (width() * height() * 4 bytes). Valid until the
   *        next call to stepFrame() / reset() / loadProgram().
   */
  virtual const uint8_t* framebuffer() const = 0;

  /**
   * @brief Set the output sample rate. Call before the first stepFrame().
   * @param hz Sample rate in Hz (e.g. 44100 or 48000).
   */
  virtual void setAudioSampleRate(int hz) = 0;

  /**
   * @brief Number of stereo frames produced by the last stepFrame().
   */
  virtual std::size_t audioFrameCount() const = 0;

  /**
   * @brief Interleaved stereo samples (L,R,L,R...) in [-1, 1], with
   *        audioFrameCount() * 2 floats. Valid until the next stepFrame().
   */
  virtual const float* audioSamples() const = 0;

  /**
   * @brief Save the current state of the emulator to a stream.
   * @return A string containing the serialized state.
   */
  virtual std::string saveState() const = 0;

  /**
   * @brief Load the state of the emulator from a stream.
   * @param json A string containing the serialized state.
   * @return true if the state was successfully loaded, false otherwise.
   */
  virtual bool loadState(const std::string& json) = 0;
};
