#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SDL2/SDL.h"
#include "game.hh"

/**
 * @file game_selector.hh
 * @brief Loads all games found in a directory and manages the currently
 *        selected game for the UI's menu.
 */

/**
 * @class GameSelector
 * @brief Scans a directory for game/ROM files, builds a Game for each one,
 *        and exposes navigation between them.
 */
class GameSelector {
 public:
  /**
   * @brief Construct a GameSelector by scanning a directory for game files.
   *
   * A Game instance is created for every regular file found in the
   * directory; the selection starts at index 0.
   *
   * @param gamesDirectory Path to the directory containing game/ROM files.
   */
  GameSelector(std::string gamesDirectory);

  /**
   * @brief Move the selection to the next game, wrapping around at the end
   *        of the list.
   */
  void nextGame();

  /**
   * @brief Move the selection to the previous game, wrapping around at the
   *        start of the list.
   */
  void previousGame();

  /**
   * @brief Load and run the currently selected game.
   * @param renderer SDL renderer the game's emulator should draw to.
   * @return The result of Game::loadGame(), or 0 if there are no games.
   */
  int selectGame(SDL_Renderer* renderer);

  /**
   * @brief Get the display names of all available games, in load order.
   * @return Vector of game names.
   */
  std::vector<std::string> getGames();

  /**
   * @brief Get the index of the currently selected game.
   * @return Index into the games list.
   */
  int getSelectedGameIndex();

 private:
  std::vector<std::unique_ptr<Game>>
      games;              ///< All games found in the games directory.
  int selectedGameIndex;  ///< Index of the currently selected game.
};
