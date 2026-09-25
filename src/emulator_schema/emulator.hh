#pragma once

#include <SDL2/SDL.h>

#include <cstdint>
#include <string>

/**
 * @file emulator.hh
 * @brief Common interface implemented by all supported system emulators
 *        (e.g. Chip8, Atari2600).
 */

/**
 * @class Emulator
 * @brief Abstract base class defining the interface a concrete emulator
 *        must implement so it can be loaded and driven by Game/GameSelector.
 */
class Emulator {
 public:
  /**
   * @brief Load a program/ROM file into the emulator, ready to be run.
   * @param filename Path to the ROM/program file to load.
   */
  virtual void loadProgram(const std::string& filename) = 0;

  /**
   * @brief Run the emulator's main execution loop.
   * @return Implementation-defined exit code. By convention, returning 1
   *         signals that the whole application should quit.
   */
  virtual int run() = 0;

  /**
   * @brief Provide the SDL renderer the emulator should draw to.
   * @param renderer SDL renderer to use for video output.
   */
  virtual void setRenderer(SDL_Renderer* renderer) = 0;
};
