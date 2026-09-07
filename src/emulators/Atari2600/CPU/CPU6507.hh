#include <cstdint>

#include <array>

class CPU6507 {
    public:

        CPU6507();

        void reset();
        void step();

        uint8_t A;
        uint8_t X;
        uint8_t Y;
        uint16_t SP;
        uint8_t P;

        uint16_t PC; // 13-bit program counter

        std::array<uint8_t, 0x2000> memory;
};