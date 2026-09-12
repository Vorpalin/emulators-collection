#pragma once
#include "CPU6507.hh"
#include "TIA.hh"
#include "riot.hh"
#include "cartridge_atari2600.hh"
#include "atari2600_bus.hh"
#include "emulator.hh"

class Atari2600 : public Emulator {
public:
    Atari2600();
    ~Atari2600();
    void tick();

    void loadProgram(const std::string& filename) override;
    int run() override;
    void setRenderer(SDL_Renderer* renderer) override;

private:
    Atari2600Bus bus;
};