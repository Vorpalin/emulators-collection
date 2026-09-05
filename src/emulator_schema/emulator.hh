#pragma once

#include <cstdint>
#include <string>

class Emulator {
    public:
        virtual void loadProgram(const std::string& filename) = 0;
        virtual void run() = 0;
};