#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>

#include "emulator.hh"

struct SDL_Renderer;
struct SDL_Window;

/**
 * @file chip8.hh
 * @brief CHIP-8 interpreter/emulator, implementing the Emulator interface.
 */

/**
 * @class Chip8
 * @brief Emulates a CHIP-8 system: 4K memory, 16 general registers, a
 *        64x32 (or 128x64 in high-resolution mode) monochrome display,
 *        a simple call stack, timers and a hex keypad, plus a square-wave
 *        beep via SDL audio.
 */
class Chip8 : public Emulator {
 public:
  /**
   * @brief Construct a Chip8 instance with memory, registers and the font
   *        set initialized, ready to load a program.
   */
  Chip8();

  /**
   * @brief Run the emulator's main loop (input, timers, cycles, rendering)
   *        until the program halts or the user quits.
   * @return Implementation-defined exit code; by convention 1 signals the
   *         whole application should quit.
   */
  int run() override;

  /**
   * @brief Load a CHIP-8 program/ROM into memory starting at 0x200.
   * @param filename Path to the ROM file to load.
   */
  void loadProgram(const std::string& filename) override;

  /**
   * @brief Provide the SDL renderer used to draw the display.
   * @param renderer SDL renderer to draw to.
   */
  void setRenderer(SDL_Renderer* renderer) override;

  /**
   * @brief Destroy the emulator, shutting down audio if it was initialized.
   */
  ~Chip8();

 private:
  /**
   * @brief Poll SDL events and update key states / quit flags.
   */
  void handleInput();

  /**
   * @brief Clear all key-press states in the keypad array.
   */
  void clearKeyStates();

  /**
   * @brief Decrement the delay and sound timers, run at 60 Hz independently
   *        of the CPU cycle rate.
   */
  void updateTimers();

  /**
   * @brief Render the current display buffer (gfx) to the screen.
   */
  void drawGraphics();

  /**
   * @brief Decode and execute a single fetched opcode.
   * @param opcode The 16-bit CHIP-8 opcode to execute.
   */
  void executeOpcode(uint16_t opcode);

  /**
   * @brief Fetch, decode, and execute one instruction, advancing the
   *        program counter as needed.
   */
  void cycle();

  SDL_AudioDeviceID audioDevice =
      0;  ///< SDL audio device handle for the beep tone (0 if not open).
  double gain = 0.0;   ///< Current output gain for the beep waveform.
  double phase = 0.0;  ///< Running phase of the beep square wave.
  std::atomic<bool> beepOn{false};  ///< Whether the beep is currently sounding.

  double sampleRate = 44100.0;  ///< Audio sample rate, in Hz.

  /**
   * @brief Initialize SDL audio and open the device used for the beep tone.
   */
  void initAudio();

  /**
   * @brief Close the audio device and shut down SDL audio if this instance
   *        initialized it.
   */
  void shutdownAudio();

  /**
   * @brief Turn the beep tone on or off.
   * @param on True to start beeping, false to stop.
   */
  void setBeep(bool on);

  /**
   * @brief SDL audio callback: synthesizes the beep square wave.
   * @param userdata Pointer to the owning Chip8 instance.
   * @param stream   Destination buffer, as raw bytes.
   * @param len      Length of @p stream, in bytes.
   */
  static void audioCallback(void* userdata, Uint8* stream, int len);

  std::chrono::steady_clock::time_point
      beepUntil{};  ///< Time point at which the beep should stop.

  /**
   * The CHIP-8 has 4K memory (4096 bytes).
   *
   * Memory map:
   *  - 0x000-0x1FF: CHIP-8 interpreter (contains the font set in this
   * emulator).
   *  - 0x050-0x0A0: Built-in 4x5 pixel font set (0-F).
   *  - 0x200-0xFFF: Program ROM and work RAM.
   */
  uint8_t memory[4096];

  uint8_t V[16];  ///< 16 general-purpose 8-bit registers (V0 to VF).

  uint16_t I;  ///< Index register, used for memory addressing.

  uint16_t pc;  ///< Program counter.

  uint8_t gfx[128 * 64];  ///< Display buffer (128x64 to support high-resolution
                          ///< mode).

  uint8_t draw_flag;  ///< Set when the display needs to be redrawn.

  uint8_t delay_timer;  ///< General-purpose delay timer, counts down at 60 Hz.
  uint8_t sound_timer;  ///< Sound timer; beeps while non-zero, counts down at
                        ///< 60 Hz.

  /// Call stack used to store return addresses for subroutine calls.
  uint16_t stack[16];

  uint16_t sp;  ///< Stack pointer, indexes into stack[].

  uint8_t key[16];  ///< State of the 16-key hexadecimal keypad (0x0-0xF).
  uint8_t rpl[16];  ///< RPL user flags (SCHIP persistent storage registers).

  SDL_Renderer* renderer;  ///< SDL renderer used for drawGraphics().

  bool running;  ///< Whether the emulator's main loop should keep running.
  bool quit;     ///< Set when the whole application should quit (e.g. window
                 ///< closed).

  int returnValue;  ///< Value returned by run() when the loop exits.

  bool highResolutionMode;  ///< True when SCHIP high-resolution (128x64) mode
                            ///< is active.
};
