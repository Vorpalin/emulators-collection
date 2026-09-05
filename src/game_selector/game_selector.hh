#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "SDL2/SDL.h"
#include "game.hh"

class GameSelector
{
public:
  GameSelector(std::string gamesDirectory);
  void nextGame();
  void previousGame();
  int selectGame(SDL_Renderer* renderer);

  std::vector<std::string> getGames();

  int getSelectedGameIndex();

private:
  std::vector<std::unique_ptr<Game>> games;
  int selectedGameIndex;
};