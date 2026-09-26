#pragma once

#include <array>
#include <cstdint>

#include "GameBoyBus.hh"
#include "emulator.hh"

class GameBoy : public Emulator {
 public:
  GameBoy();
  ~GameBoy();

  void loadProgram(const std::string& filename) override;
  void reset();
  int run() override;
  void setRenderer(SDL_Renderer* renderer) override {
    this->renderer = renderer;
  }

  GameBoyBus& getBus() { return bus; }

 private:
  GameBoyBus bus;
  bool isRunning = true;

  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;

  void handleInput();
  void renderFrame();
};
