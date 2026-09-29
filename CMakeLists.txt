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
    src/emulators/ppu/TIA1A/TIA1A.cc
    src/emulators/ppu/game_boy_ppu/GameBoyPPU.cc
    src/emulators/bus/atari2600_bus/Atari2600Bus.cc
    src/emulators/bus/game_boy_bus/GameBoyBus.cc
    src/emulators/cpu/CPU65/CPU65.cc
    src/emulators/timer/game_boy_timer/GameBoyTimer.cc
    src/emulators/cpu/CPU65/CPU65.cc
    src/emulators/cpu/CPU65/setFlags.cc
    src/emulators/cpu/LR35902/LR35902.cc
    src/emulators/cpu/LR35902/instructionSet.cc
    src/emulators/cpu/LR35902/prefixInstructionSet.cc
    src/emulators/cartridge/Cartridge.cc
    src/emulators/cartridge/game_boy_cartridge/GameBoyCartridge.cc
    src/emulators/processor/MOS6532/MOS6532.cc
    src/main.cc
    src/emulators/console/atari2600/Atari2600.cc
    src/emulators/console/chip8/Chip8.cc
    src/emulators/console/game_boy/GameBoy.cc
    src/game/Game.cc
    src/game_selector/GameSelector.cc
    src/ui/UI.cc

)

target_include_directories(emulators-collection
    PRIVATE
        src/
)

target_compile_options(emulators-collection
    PRIVATE
        -O3
        -Wall
        -Wextra
        -Werror
        -pedantic
        -Wold-style-cast
)

target_link_libraries(emulators-collection
    PRIVATE
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
)
