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
    src/emulators/chip/TIA1A/TIA1A.cc
    src/emulators/BUS/Atari2600Bus/Atari2600Bus.cc
    src/emulators/CPU/CPU65/CPU65.cc
    src/emulators/CPU/CPU65/setFlags.cc
    src/emulators/CPU/SM83/SM83.cc
    src/emulators/CPU/SM83/instructionSet.cc
    src/emulators/CPU/SM83/prefixInstructionSet.cc
    src/emulators/Cartbridge/Cartbridge.cc
    src/emulators/Processor/MOS6532/MOS6532.cc
    src/main.cc
    src/emulators/console/Atari2600/Atari2600.cc
    src/emulators/console/chip_8/chip8.cc
    src/game/game.cc
    src/game_selector/game_selector.cc
    src/UI/UI.cc

)

target_include_directories(emulators-collection
    PRIVATE
        src/emulators/CPU/
        src/emulators/CPU/CPU65
        src/emulators/CPU/SM83
        src/emulators/BUS
        src/emulators/BUS/Atari2600Bus
        src/emulators/chip/TIA1A
        src/emulators/Cartbridge
        src/emulators/Processor/MOS6532
        src/emulators/console/Atari2600
        src/emulators/console/chip_8
        src/emulator_schema
        src/game
        src/game_selector
        src/UI
)

target_compile_options(emulators-collection
    PRIVATE
        -Wall
        -Wextra
        -Werror
        -pedantic
        -Wold-style-cast
        -std=c++20
)

target_link_libraries(emulators-collection
    PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)
