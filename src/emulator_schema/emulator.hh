#pragma once

#include <SDL2/SDL.h>

#include <cstdint>
#include <string>

class Emulator {
 public:
  virtual void loadProgram(const std::string& filename) = 0;
  virtual int run() = 0;
  virtual void setRenderer(SDL_Renderer* renderer) = 0;
};
