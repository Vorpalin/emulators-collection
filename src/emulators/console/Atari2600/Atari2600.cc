#include "Atari2600.hh"

Atari2600::Atari2600() : bus(), renderer(nullptr), isRunning(true) {
    // Initialize the Atari 2600 emulator
}

void Atari2600::loadProgram(const std::string& filename) {
    bus.loadROM(const_cast<std::string&>(filename));
}

void Atari2600::setRenderer(SDL_Renderer* renderer) {
    this->renderer = renderer;
}

void Atari2600::reset() {
    bus.reset();
    isRunning = true; // Set the running state to true after reset
    this->renderer = nullptr; // Reset the renderer to nullptr after reset
}

int Atari2600::run() {
    while (isRunning) {
        bus.tick(); // Advance the bus by one clock cycle
    }
    return 0;
}