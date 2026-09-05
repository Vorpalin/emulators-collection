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
        void updateFont(int winH);

        GameSelector gameSelector;

        std::vector<std::string> games;

        SDL_Window* window;
        SDL_Renderer* renderer;

        bool running;
        std::string fontPath = "assets/fonts/PressStart2P-Regular.ttf";
        int currentFontSize = 6;

        TTF_Font* font;
};