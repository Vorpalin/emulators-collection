#include <cstdint>
#include <SDL2/SDL.h>

class TIA {
public:
    TIA();
    void write(uint8_t reg, uint8_t value);
    uint8_t read(uint8_t reg);
    void reset();

    void tick();

    void setRenderer(SDL_Renderer* renderer);

private:
    uint8_t registers[64];

    uint16_t scanline;
    uint8_t cycle;

    SDL_Renderer* renderer; // Pointer to the SDL renderer for rendering graphics
};