#include "Atari2600.hh"

#include <SDL2/SDL.h>

#include <cstdint>
#include <vector>

Atari2600::Atari2600()
    : bus(), renderer(nullptr), frameTexture(nullptr), isRunning(true) {
    // Initialize the Atari 2600 emulator
}

Atari2600::~Atari2600() {
    if (frameTexture)
        SDL_DestroyTexture(frameTexture);
}

void Atari2600::loadProgram(const std::string& filename) {
    bus.loadROM(const_cast<std::string&>(filename));
    bus.reset();
}

void Atari2600::setRenderer(SDL_Renderer* renderer) {
    this->renderer = renderer;
}

void Atari2600::renderFrame() {
    if (!renderer)
        return;

    if (!frameTexture) {
        frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                         SDL_TEXTUREACCESS_STREAMING,
                                         TIA1A::kWidth, TIA1A::kHeight);
        if (!frameTexture)
            return;

        SDL_SetTextureScaleMode(frameTexture, SDL_ScaleModeNearest);
        SDL_RenderSetLogicalSize(renderer, TIA1A::kWidth, TIA1A::kHeight);
    }

    std::vector<uint32_t> pixels(TIA1A::kWidth * TIA1A::kHeight);
    const uint8_t* frame = bus.frame();
    for (size_t i = 0; i < pixels.size(); ++i) {
        const uint8_t intensity = static_cast<uint8_t>(
            (static_cast<unsigned int>(frame[i] & 0x7F) * 255) / 0x7F);
        pixels[i] = 0xFF000000u | (static_cast<uint32_t>(intensity) << 16) |
                    (static_cast<uint32_t>(intensity) << 8) | intensity;
    }

    SDL_UpdateTexture(frameTexture, nullptr, pixels.data(),
                      TIA1A::kWidth * static_cast<int>(sizeof(uint32_t)));
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
}

void Atari2600::reset() {
    bus.reset();
    isRunning = true; // Set the running state to true after reset
}

int Atari2600::run() {
    while (isRunning) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT ||
                (event.type == SDL_KEYDOWN &&
                 event.key.keysym.sym == SDLK_ESCAPE)) {
                isRunning = false;
            }
        }

        for (int cycle = 0; cycle < 10000 && isRunning && !bus.frameReady();
             ++cycle) {
            bus.tick();
        }

        if (bus.frameReady()) {
            renderFrame();
            bus.clearFrameReady();
        }
    }
    return 0;
}