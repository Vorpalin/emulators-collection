#include "atari2600.hh"


Atari2600::Atari2600() : bus() {
    // Initialize the Atari 2600 emulator
}

Atari2600::~Atari2600() {
    // Clean up the Atari 2600 emulator
}

void Atari2600::tick() {
    // Perform a single tick of the Atari 2600 emulator
    bus.tick();
}

void Atari2600::loadProgram(const std::string& filename) {
    // Load the Atari 2600 program (ROM) into the emulator
    bus.loadCartridge(filename);
}

void Atari2600::setRenderer(SDL_Renderer* renderer) {
    bus.setRenderer(renderer); // Set the SDL renderer for TIA rendering
}

int Atari2600::run() {
    // Main loop for running the Atari 2600 emulator
    // This would typically involve repeatedly calling tick() and handling events
    while (true) {
        this->tick();
        // Handle SDL events, rendering, etc.
    }
    return 0; // Return an appropriate exit code
}