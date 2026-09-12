#pragma once

#include <cstdint>
#include <array>
#include "SDL2/SDL.h"
#include "CPU6507.hh"
#include "riot.hh"
#include "TIA.hh"
#include "cartridge_atari2600.hh"

class CPU6507;

class Atari2600Bus {
public:
    Atari2600Bus(CPU6507* cpu);
    ~Atari2600Bus();
    void loadCartridge(const std::string& path);
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);
    void tick();
    void setRenderer(SDL_Renderer* renderer); // Set the SDL renderer for TIA rendering

private:
    CPU6507* cpu;
    TIA* tia;
    RIOT* riot;
    CartridgeAtari2600* cartridge;
};