#include "UI.hh"

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <string>


UI::UI()
    : gameSelector("games")
{
    SDL_Init(SDL_INIT_VIDEO);

    this->window = SDL_CreateWindow("Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 480, SDL_WINDOW_SHOWN);
    this->renderer = SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);

    this->running = true;
    this->gameSelector = GameSelector("games");

    this->games = this->gameSelector.getGames();

    if (TTF_Init() == -1)
    {
        std::cerr << "Failed to init SDL_ttf: "
              << TTF_GetError()
              << std::endl;

    }

    this->font = TTF_OpenFont(
        "assets/fonts/PressStart2P-Regular.ttf",
        24
    );

    if (!this->font)
    {
        std::cerr << "Failed to load font: "
                  << TTF_GetError()
                  << std::endl;
    }
}

void UI::handleInput()
{
    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            this->running = false;
            return;
        }

        if (event.type != SDL_KEYDOWN)
            continue;

        switch (event.key.keysym.sym)
        {
            case SDLK_UP:
                this->gameSelector.previousGame();
                break;

            case SDLK_DOWN:
                this->gameSelector.nextGame();
                break;

            case SDLK_RETURN:
                this->gameSelector.selectGame();
                break;

            case SDLK_ESCAPE:
                this->running = false;
                break;
        }
    }
}
void UI::drawText(
    const std::string& text,
    int x,
    int y)
{
    SDL_Color white{255, 255, 255, 255};

    SDL_Surface* surface = TTF_RenderText_Solid(
        this->font,
        text.c_str(),
        white
    );


    if (!surface)
        return;

    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(
            this->renderer,
            surface
        );

    SDL_Rect destination{
        x,
        y,
        surface->w,
        surface->h
    };

    SDL_RenderCopy(
        this->renderer,
        texture,
        nullptr,
        &destination
    );

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);
}

void UI::render()
{
    SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
    SDL_RenderClear(this->renderer);

    this->drawText(
        "Emulator Collection",
        250,
        50
    );

    int selected = this->gameSelector.getSelectedGameIndex();

    int startX = 100;
    int startY = 150;
    int spacing = 40;

    for (size_t i = 0; i < games.size(); ++i)
    {
        int x = startX;
        int y = startY + i * spacing;

        // Selection background
        if (static_cast<int>(i) == selected)
        {
            SDL_Rect selection{
                x - 20,
                y - 5,
                400,
                35
            };

            SDL_SetRenderDrawColor(
                this->renderer,
                50, 50, 50, 255
            );

            SDL_RenderFillRect(
                this->renderer,
                &selection
            );
        }

        // Selection indicator
        std::string prefix =
            (static_cast<int>(i) == selected)
                ? "> "
                : "  ";

        this->drawText(
            prefix + this->games[i],
            x,
            y
        );
    }

    this->drawText(
        "UP / DOWN : Select",
        100,
        450
    );

    this->drawText(
        "ENTER : Launch",
        100,
        480
    );

    this->drawText(
        "ESC : Quit",
        100,
        510
    );

    SDL_RenderPresent(this->renderer);
}
void UI::run()
{
    this->running = true;

    while (this->running)
    {
        this->handleInput();
        this->render();

        SDL_Delay(16);
    }
}