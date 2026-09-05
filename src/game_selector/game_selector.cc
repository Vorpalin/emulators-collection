#include "game_selector.hh"
#include <filesystem>


GameSelector::GameSelector(std::string gamesDirectory)
    : selectedGameIndex(0)
{
    for (const auto& entry : std::filesystem::directory_iterator(gamesDirectory))
    {
        if (!entry.is_regular_file())
            continue;

        games.push_back(std::unique_ptr<Game>(new Game(entry.path().filename().string(), entry.path().parent_path().string())));
    }
}

void GameSelector::nextGame()
{
    selectedGameIndex = (selectedGameIndex + 1) % games.size();
}

void GameSelector::previousGame()
{
    selectedGameIndex = (selectedGameIndex - 1 + games.size()) % games.size();
}

std::vector<std::string> GameSelector::getGames()
{
    std::vector<std::string> gameNames;
    for (const auto& game : games)
    {
        gameNames.push_back(game->getName());
    }
    return gameNames;
}

int GameSelector::getSelectedGameIndex()
{
    return selectedGameIndex;
}

void GameSelector::selectGame()
{
    if (games.empty())
        return;

    games[selectedGameIndex]->loadGame();
}