#pragma once

#include <cstdint>
#include "atari2600_bus.hh"
#include <string>

#include <array>

class Atari2600Bus;

class CPU6507 {
    public:

        CPU6507();
        ~CPU6507();

        void reset();
        void tick();
        void step();
        void setRenderer(SDL_Renderer* renderer);
        void loadProgram(const std::string& filename);

        uint8_t A; // accumulator
        uint8_t X; // index register X
        uint8_t Y; // index register Y
        uint8_t SP; // stack pointer
        uint8_t P; // processor status register

        uint8_t SR; // status register (for flags)
        uint16_t PC; // 13-bit program counter

        std::array<uint8_t, 0x2000> memory;

    private:
        Atari2600Bus* bus;
        
        void setFlagZ(uint8_t value);
        void setFlagN(uint8_t value);
        void setFlagC(uint8_t value);
        void setFlagV(uint8_t value);
        void setFlagD(uint8_t value);
        void setFlagI(uint8_t value);
        void setFlagB(uint8_t value);
};