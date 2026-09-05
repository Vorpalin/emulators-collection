#pragma once

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "game_selector.hh"

class UI
{
    public:
        UI();

        void run();
        
    private:
        void handleInput();
        void render();
        void drawText(const std::string& text, int x, int y);

        GameSelector gameSelector;

        std::vector<std::string> games;

        SDL_Window* window;
        SDL_Renderer* renderer;

        bool running;

        TTF_Font* font;
};