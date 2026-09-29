#pragma once

#include <array>
#include <cstdint>

#include "GameBoyBus.hh"
#include "emulator.hh"

/**
 * @file GameBoy.hh
 * @brief Top-level Game Boy emulator (SDL front-end).
 */

/**
 * @class GameBoy
 * @brief Top-level Game Boy system.
 *
 * Owns the GameBoyBus (which in turn owns the CPU, PPU, cartridge, timer,
 * joypad...) and implements the Emulator interface: ROM loading, main loop,
 * input handling and rendering through SDL.
 */
class GameBoy : public Emulator {
 public:
  /** @brief Constructs the emulator and its bus. */
  GameBoy();
  /** @brief Destroys the emulator and releases SDL resources. */
  ~GameBoy();

  /**
   * @brief Loads a ROM file into the cartridge.
   * @param filename Path to the `.gb` ROM file.
   */
  void loadProgram(const std::string& filename) override;

  /** @brief Resets the whole system (bus, CPU, PPU, ...) to power-on state. */
  void reset();

  /**
   * @brief Runs the main emulation loop until the user quits.
   * @return Process-style exit code (0 on success).
   */
  int run() override;

  /**
   * @brief Sets the SDL renderer used to display frames.
   * @param renderer SDL renderer (not owned).
   */
  void setRenderer(SDL_Renderer* renderer) override {
    this->renderer = renderer;
  }

  /** @brief Gives access to the system bus (useful for debugging/tests). */
  GameBoyBus& getBus() { return bus; }

 private:
  GameBoyBus bus;         ///< System bus, owner of all hardware components.
  bool isRunning = true;  ///< Main loop flag; set to false to quit.

  SDL_Renderer* renderer = nullptr;  ///< SDL renderer (not owned).
  SDL_Texture* texture = nullptr;    ///< Streaming texture holding the frame.

  /** @brief Polls SDL events and forwards key states to the joypad. */
  void handleInput();
  /** @brief Uploads the PPU framebuffer to the texture and presents it. */
  void renderFrame();
};
