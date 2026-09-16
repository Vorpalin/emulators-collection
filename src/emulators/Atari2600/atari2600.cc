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

#include <iostream>
int Atari2600::run() {
    bool quit = false;

    bool running = true; // Flag to control the execution of the emulator
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        uint8_t swcha = 0xFF;
        if (keys[SDL_SCANCODE_ESCAPE])
            quit = true; // Exit the emulator if Escape is pressed
        if (keys[SDL_SCANCODE_UP])    swcha &= ~0x08; // P0 haut
        if (keys[SDL_SCANCODE_DOWN])  swcha &= ~0x04; // P0 bas
        if (keys[SDL_SCANCODE_LEFT])  swcha &= ~0x02; // P0 gauche
        if (keys[SDL_SCANCODE_RIGHT]) swcha &= ~0x01; // P0 droite
        cpu.setSwitchA(swcha);

        uint8_t swchb = 0xFF; // couleur + difficulté B par défaut (tous les bits à 1)
        if (keys[SDL_SCANCODE_F1]) swchb &= ~0x01; // Reset
        if (keys[SDL_SCANCODE_F2]) swchb &= ~0x02; // Select
        cpu.setSwitchB(swchb);

        cpu.setInputPort(4, keys[SDL_SCANCODE_SPACE] ? 0x00 : 0xFF); // Fire button for Player 0
        this->tick();
    }

    return 0;
}