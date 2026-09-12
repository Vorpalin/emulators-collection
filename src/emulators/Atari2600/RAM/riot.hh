#include <array>
#include <cstdint>

class RIOT {

    public:
        RIOT();

        void reset();
        void write(uint16_t address, uint8_t value);
        uint8_t read(uint16_t address);
        std::array<uint8_t, 128> ram;


};