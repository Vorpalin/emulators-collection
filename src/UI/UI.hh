#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "game_selector.hh"

/**
 * @file UI.hh
 * @brief Main application window: renders the game selection menu and
 *        handles user input via SDL2.
 */

/**
 * @class UI
 * @brief Owns the SDL2 window/renderer/font, the GameSelector, and drives
 *        the main input/render loop of the application.
 */
class UI {
 public:
  /**
   * @brief Initialize SDL2, SDL_ttf, the window, renderer and font, and
   *        load the list of available games from the "games" directory.
   */
  UI();

  /**
   * @brief Run the main application loop (input handling + rendering)
   *        until the user quits or a launched game signals exit.
   */
  void run();

 private:
  /**
   * @brief Poll and dispatch pending SDL events (navigation, launch, quit).
   */
  void handleInput();

  /**
   * @brief Render one frame: title, game list with selection, and hints.
   */
  void render();

  /**
   * @brief Draw a line of text at the given position with the current font.
   * @param text Text to draw.
   * @param x    X coordinate, in pixels.
   * @param y    Y coordinate, in pixels.
   */
  void drawText(const std::string& text, int x, int y);

  /**
   * @brief Recompute the font size based on window height and reload the
   *        font if the size changed.
   * @param winH Current window height, in pixels.
   */
  void updateFont(int winH);

  GameSelector
      gameSelector;  ///< Manages the list of games and the current selection.

  std::vector<std::string>
      games;  ///< Cached display names of all available games.

  SDL_Window* window;      ///< Main application window.
  SDL_Renderer* renderer;  ///< Renderer used for all drawing.

  bool running;  ///< Whether the main loop should keep running.
  std::string fontPath =
      "assets/fonts/PressStart2P-Regular.ttf";  ///< Path to the TTF font asset.
  int currentFontSize = 6;  ///< Currently loaded font point size.

  TTF_Font* font;  ///< Currently loaded font, used by drawText().
};
