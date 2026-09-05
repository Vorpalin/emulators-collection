cmake_minimum_required(VERSION 3.20)

project(emulators-collection
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL2 REQUIRED)

add_executable(emulators-collection
    src/main.cc
    src/CHIP-8/chip8.cc
)

target_include_directories(emulators-collection
    PRIVATE
        src/CHIP-8
        src/emulator_schema
)

target_link_libraries(emulators-collection
    PRIVATE
        SDL2::SDL2
)