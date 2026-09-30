#include "game/Game.hh"

#include <iostream>
#include <memory>
#include <string>

#include "emulators/console/atari2600/Atari2600.hh"
#include "emulators/console/chip8/Chip8.hh"
#include "emulators/console/game_boy/GameBoy.hh"

Game::Game(const std::string& file, const std::string& dir)
    : path(dir + "/" + file) {
  size_t lastindex = file.find_last_of(".");
  name = file.substr(0, lastindex);

  std::string extension = file.substr(lastindex + 1);

  if (extension == "ch8") {
    emulatorFactory = []() { return std::make_unique<Chip8>(); };
  } else if (extension == "a26") {
    emulatorFactory = []() { return std::make_unique<Atari2600>(); };
  } else if (extension == "gb") {
    emulatorFactory = []() { return std::make_unique<GameBoy>(); };
  } else {
    std::cerr << "Unsupported file extension: " << extension << std::endl;
    emulatorFactory = nullptr;
  }
}

std::string Game::getName() { return name; }

int Game::loadGame(SDL_Renderer* renderer) {
  if (emulatorFactory) {
    auto emulator = emulatorFactory();
    emulator->setRenderer(renderer);
    emulator->loadProgram(this->path);
    int result = emulator->run();
    return result;
  }
  return 0;
}
