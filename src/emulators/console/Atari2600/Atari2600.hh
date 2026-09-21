#pragma once

#include "emulator.hh"

#include "Atari2600Bus.hh"

struct SDL_Renderer;
struct SDL_Window;

class Atari2600 : public Emulator {

    public:
        Atari2600();
        ~Atari2600();
        int run() override;

        void loadProgram(const std::string& filename) override;
        void setRenderer(SDL_Renderer* renderer) override;

        void reset();
    private:
        Atari2600Bus bus;
        SDL_Renderer* renderer;
        SDL_Texture* frameTexture;

        bool isRunning;

        void renderFrame();
};