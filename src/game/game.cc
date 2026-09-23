#include "game.hh"

#include <iostream>
#include <memory>
#include <string>

#include "Atari2600.hh"
#include "chip8.hh"

Game::Game(const std::string& file, const std::string& dir)
    : path(dir + "/" + file) {}
{
  size_t lastindex = file.find_last_of(".");
  name = file.substr(0, lastindex);

  std::string extension = file.substr(lastindex + 1);

  if (extension == "ch8") {
    emulator = std::unique_ptr<Chip8>(new Chip8());
  } else if (extension == "a26") {
    emulator = std::unique_ptr<Atari2600>(new Atari2600());
  } else {
    std::cerr << "Unsupported file extension: " << extension << std::endl;
    emulator = nullptr;
  }
}

std::string Game::getName() { return name; }

int Game::loadGame(SDL_Renderer* renderer) {
  if (emulator) {
    emulator->setRenderer(renderer);
    emulator->loadProgram(this->path);
    return emulator->run();
  }
  return 0;
}
