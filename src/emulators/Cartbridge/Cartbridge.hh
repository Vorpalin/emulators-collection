#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Cartbridge {
    public:
        Cartbridge();

        void loadROM(std::string &filename);
        void reset();
    private:
        std::vector<uint8_t> romData; // Vector to hold the ROM data
};