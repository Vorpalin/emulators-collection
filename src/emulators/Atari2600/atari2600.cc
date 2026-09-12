#include "atari2600.hh"
#include <SDL2/SDL.h>

Atari2600::Atari2600() : cpu() {
    // Initialize the Atari 2600 emulator
}

Atari2600::~Atari2600() {
    // Clean up the Atari 2600 emulator
}

void Atari2600::tick() {
    // Perform a single tick of the Atari 2600 emulator
    cpu.tick();
}

void Atari2600::loadProgram(const std::string& filename) {
    // Load the Atari 2600 program (ROM) into the emulator
    cpu.loadProgram(filename);
}

void Atari2600::setRenderer(SDL_Renderer* renderer) {
    cpu.setRenderer(renderer); // Set the SDL renderer for TIA rendering
}

int Atari2600::run() {
    bool quit = false;

    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
                break;
            }

            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
                break;
            }
        }

        this->tick();
        SDL_Delay(1);
    }

    return 0;
}