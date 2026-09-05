cmake_minimum_required(VERSION 3.20)

project(emulators-collection
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)
find_package(SDL2_ttf REQUIRED)

add_executable(emulators-collection
    src/main.cc
    src/emulators/chip_8/chip8.cc
    src/game/game.cc
    src/game_selector/game_selector.cc
    src/UI/UI.cc
)

target_include_directories(emulators-collection
    PRIVATE
        src/emulators/chip_8
        src/emulator_schema
        src/game
        src/game_selector
        src/UI
)

target_link_libraries(emulators-collection
    PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)
