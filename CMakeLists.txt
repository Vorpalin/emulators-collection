cmake_minimum_required(VERSION 3.20)

project(emulators-collection
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ============================================================
# Dependencies
# ============================================================

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(nlohmann_json)

# ============================================================
# Common options
# ============================================================

option(WARNINGS_AS_ERRORS "Treat compiler warnings as errors (enabled in CI)" OFF)

add_library(emulator_options INTERFACE)

set(EMULATOR_WARNINGS
    -Wall
    -Wextra
    -pedantic
    -Wold-style-cast
)

target_compile_options(emulator_options
    INTERFACE
        "$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:${EMULATOR_WARNINGS}>"
        "$<$<AND:$<BOOL:${WARNINGS_AS_ERRORS}>,$<CXX_COMPILER_ID:GNU,Clang,AppleClang>>:-Werror>"
)

target_include_directories(emulator_options
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ============================================================
# CHIP-8
# ============================================================

add_library(chip8 STATIC
    src/emulators/console/chip8/Chip8.cc
)

target_link_libraries(chip8
    PUBLIC
        emulator_options
        nlohmann_json::nlohmann_json
)

# ============================================================
# Atari 2600
# ============================================================

add_library(atari2600 STATIC
    src/emulators/console/atari2600/Atari2600.cc
    src/emulators/audio/TIA1A/TIAAudio.cc
    src/emulators/cpu/CPU65/CPU65.cc
    src/emulators/cpu/CPU65/setFlags.cc
    src/emulators/ppu/TIA1A/TIA1A.cc
    src/emulators/bus/atari2600_bus/Atari2600Bus.cc
    src/emulators/cartridge/atari2600_cartridge/Atari2600Cartridge.cc
    src/emulators/processor/MOS6532/MOS6532.cc
)

target_link_libraries(atari2600
    PUBLIC
        emulator_options
        nlohmann_json::nlohmann_json
)

# ============================================================
# Game Boy
# ============================================================

add_library(gameboy STATIC
    src/emulators/console/game_boy/GameBoy.cc
    src/emulators/cpu/LR35902/LR35902.cc
    src/emulators/cpu/LR35902/instructionSet.cc
    src/emulators/cpu/LR35902/prefixInstructionSet.cc
    src/emulators/ppu/game_boy_ppu/GameBoyPPU.cc
    src/emulators/bus/game_boy_bus/GameBoyBus.cc
    src/emulators/timer/game_boy_timer/GameBoyTimer.cc
    src/emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.cc
    src/emulators/cartridge/game_boy_cartridge/GameBoyCartridge.cc
    src/emulators/controller/game_boy_controller/GameBoyController.cc
    src/emulators/audio/APU/APU.cc
)

target_link_libraries(gameboy
    PUBLIC
        emulator_options
        nlohmann_json::nlohmann_json
)

# ============================================================
# Main application
# ============================================================

if(EMSCRIPTEN)

    add_executable(emulators
        src/wasm/bindings.cc
    )

    target_link_libraries(emulators
        PRIVATE
            emulator_options
            chip8
            atari2600
            gameboy
    )

    target_link_options(emulators
        PRIVATE
            --bind
            -sMODULARIZE=1
            -sEXPORT_ES6=1
            -sALLOW_MEMORY_GROWTH=1
            -sEXPORTED_RUNTIME_METHODS=HEAPU8,HEAPF32
    )

endif()
