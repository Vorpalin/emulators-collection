#pragma once

#include <cstdint>
#include <string>
#include <SDL2/SDL.h>

class Emulator
{
public:
  virtual void loadProgram(const std::string& filename) = 0;
  virtual int run() = 0;
  virtual void setRenderer(SDL_Renderer* renderer) = 0;
};