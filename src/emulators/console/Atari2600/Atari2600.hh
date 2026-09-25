#pragma once

#include "Atari2600Bus.hh"
#include "TIAAudio.hh"
#include "emulator.hh"

struct SDL_Renderer;
struct SDL_Window;

/**
 * @file Atari2600.hh
 * @brief Top-level Atari 2600 emulator, implementing the Emulator interface
 *        by driving the system bus, presenting its video output via an
 *        SDL texture, and forwarding audio register writes to TIAAudio.
 */

/**
 * @class Atari2600
 * @brief Emulates an Atari 2600 console: owns the system bus (CPU, RIOT,
 *        TIA, cartridge), an SDL texture used to display the TIA's frame
 *        buffer, and the TIA audio engine.
 */
class Atari2600 : public Emulator {
 public:
  /**
   * @brief Construct the emulator and wire up the audio write hook between
   *        the bus and the TIAAudio engine.
   */
  Atari2600();

  /**
   * @brief Destroy the emulator, releasing SDL resources (texture, audio).
   */
  ~Atari2600();

  /**
   * @brief Run the emulator's main loop: step the bus, present completed
   *        frames, and update audio, until the program halts or the user
   *        quits.
   * @return Implementation-defined exit code; by convention 1 signals the
   *         whole application should quit.
   */
  int run() override;

  /**
   * @brief Load a cartridge ROM into the bus, ready to be run.
   * @param filename Path to the ROM file to load.
   */
  void loadProgram(const std::string& filename) override;

  /**
   * @brief Provide the SDL renderer used to display the emulated video
   *        output, and (re)create the frame texture for it.
   * @param renderer SDL renderer to draw to.
   */
  void setRenderer(SDL_Renderer* renderer) override;

  /**
   * @brief Reset the bus (CPU, RIOT, TIA, cartridge) to its power-up state.
   */
  void reset();

 private:
  Atari2600Bus bus;           ///< System bus: CPU, cartridge, RIOT and TIA.
  SDL_Renderer* renderer;     ///< SDL renderer used to present frames.
  SDL_Texture* frameTexture;  ///< Texture the TIA frame buffer is uploaded into
                              ///< each frame.
  TIAAudio audio;  ///< TIA audio engine, fed via the bus's audio write hook.

  bool isRunning;  ///< Whether the main loop should keep running.

  /**
   * @brief Upload the TIA's current frame buffer into frameTexture and
   *        present it via the renderer.
   */
  void renderFrame();
};
