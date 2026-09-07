#include <cstdint>

class TIA {
public:
    void write(uint8_t reg, uint8_t value);
    uint8_t read(uint8_t reg);

    void tick();

private:
    uint8_t registers[64];

    uint16_t scanline;
    uint8_t cycle;
};