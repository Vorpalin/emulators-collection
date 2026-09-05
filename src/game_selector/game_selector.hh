#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "game.hh"

class GameSelector
{
public:
    GameSelector(std::string gamesDirectory);
    void nextGame();
    void previousGame();
    void selectGame();

    std::vector<std::string> getGames();

    int getSelectedGameIndex();

private:

    std::vector<std::unique_ptr<Game>> games;
    int selectedGameIndex;
};