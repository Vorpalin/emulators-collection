#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "SDL2/SDL.h"
#include "emulator.hh"

/**
 * @file game.hh
 * @brief Represents a single playable game/ROM file and owns the emulator
 *        instance used to run it.
 */

/**
 * @class Game
 * @brief Wraps a ROM file on disk together with the Emulator implementation
 *        appropriate for its file extension (e.g. ".ch8" -> Chip8,
 *        ".a26" -> Atari2600).
 */
class Game {
 public:
  /**
   * @brief Construct a Game from a file name and its containing directory.
   *
   * The file extension determines which Emulator implementation is
   * instantiated. If the extension is not recognized, no emulator is
   * created and loadGame() becomes a no-op.
   *
   * @param file Name of the ROM file (including extension), without the
   *             directory path.
   * @param dir  Directory containing the ROM file.
   */
  Game(const std::string& file, const std::string& dir);

  /**
   * @brief Get the display name of the game (file name without extension).
   * @return The game's name.
   */
  std::string getName();

  /**
   * @brief Load the ROM into the underlying emulator and run it.
   * @param renderer SDL renderer the emulator should draw to.
   * @return The emulator's run() result, or 0 if no emulator was created
   *         for this game's file type.
   */
  int loadGame(SDL_Renderer* renderer);

 private:
  std::string name;  ///< Display name (file name without extension).
  std::string path;  ///< Full path to the ROM file on disk.

  std::unique_ptr<Emulator>
      emulator;  ///< Emulator instance used to run the game.
};
