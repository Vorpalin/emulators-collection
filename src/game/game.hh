#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "SDL2/SDL.h"
#include "emulator.hh"

class Game {
 public:
  Game(const std::string& file, const std::string& dir);

  std::string getName();

  int loadGame(SDL_Renderer* renderer);

 private:
  std::string name;
  std::string path;

  std::unique_ptr<Emulator> emulator;
};
