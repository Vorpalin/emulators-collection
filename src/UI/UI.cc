#include "UI.hh"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <SDL2/SDL.h>

UI::UI()
  : gameSelector("games")
{
  SDL_Init(SDL_INIT_VIDEO);

  this->window =
    SDL_CreateWindow("Emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                     640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  this->renderer =
    SDL_CreateRenderer(this->window, -1, SDL_RENDERER_ACCELERATED);

  this->running = true;
  this->gameSelector = GameSelector("games");

  this->games = this->gameSelector.getGames();

  if (TTF_Init() == -1)
    {
      std::cerr << "Failed to init SDL_ttf: " << TTF_GetError() << std::endl;
    }

  this->font = TTF_OpenFont(this->fontPath.c_str(), this->currentFontSize);

  if (!this->font)
    {
      std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
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
          if (this->gameSelector.selectGame(this->renderer) == 1)
            {
              this->running = false;
            }
          SDL_RenderSetLogicalSize(this->renderer, 0, 0);
          break;

        case SDLK_ESCAPE:
          this->running = false;
          break;
        }
    }
}
void UI::drawText(const std::string& text, int x, int y)
{
  SDL_Color white{255, 255, 255, 255};

  SDL_Surface* surface = TTF_RenderText_Solid(this->font, text.c_str(), white);

  if (!surface)
    return;

  SDL_Texture* texture = SDL_CreateTextureFromSurface(this->renderer, surface);

  SDL_Rect destination{x, y, surface->w, surface->h};

  SDL_RenderCopy(this->renderer, texture, nullptr, &destination);

  SDL_DestroyTexture(texture);
  SDL_FreeSurface(surface);
}

void UI::updateFont(int winH)
{
  int newSize = winH / 30; // ratio à ajuster selon le rendu voulu
  if (newSize < 6)
    newSize = 6;
  if (newSize > 24)
    newSize = 24;

  if (newSize == this->currentFontSize && this->font)
    return;

  TTF_Font* newFont = TTF_OpenFont(this->fontPath.c_str(), newSize);
  if (!newFont)
    {
      std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
      return;
    }

  if (this->font)
    TTF_CloseFont(this->font);

  this->font = newFont;
  this->currentFontSize = newSize;
}

void UI::render()
{
  int winW, winH;
  SDL_GetWindowSize(this->window, &winW, &winH);
  this->updateFont(winH);

  SDL_SetRenderDrawColor(this->renderer, 0, 0, 0, 255);
  SDL_RenderClear(this->renderer);

  this->drawText("Emulator Collection", winW / 2.5, winH * 0.10);

  int selected = this->gameSelector.getSelectedGameIndex();
  int totalGames = static_cast<int>(this->games.size());

  int listTop = winH * 0.25;
  int listBottom = winH * 0.85;
  int listHeight = listBottom - listTop;

  int spacing = std::max(this->currentFontSize + 10, 20);
  int visibleCount = std::max(listHeight / spacing, 1);
  visibleCount =
    std::min(visibleCount, totalGames); // pas plus de lignes que de jeux

  int startX = winW * 0.10;

  if (totalGames > 0)
    {
      for (int row = 0; row < visibleCount; ++row)
        {
          int gameIndex = (selected + row) % totalGames;
          int y = listTop + row * spacing;

          // Le premier affiché (row 0) est toujours le sélectionné
          if (row == 0)
            {
              SDL_Rect selectionRect{
                startX - 20, y - 5, static_cast<int>(winW * 0.55), spacing - 5};
              SDL_SetRenderDrawColor(this->renderer, 50, 50, 50, 255);
              SDL_RenderFillRect(this->renderer, &selectionRect);
            }

          std::string prefix = (row == 0) ? "> " : "  ";
          this->drawText(prefix + this->games[gameIndex], startX, y);
        }
    }

  this->drawText("UP / DOWN : Select", winW * 0.07, winH * 0.90);
  this->drawText("ENTER : Launch", winW * 0.07, winH * 0.94);
  this->drawText("ESC : Quit", winW * 0.58, winH * 0.90);

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