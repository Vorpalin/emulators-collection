#pragma once

#include <cstdint>
#include <vector>
#include <SDL2/SDL.h>

class TIA {
public:
    static constexpr int ScreenWidth = 160;
    static constexpr int ScreenHeight = 192;

    TIA();
    ~TIA();

    void write(uint8_t reg, uint8_t value);
    uint8_t read(uint8_t reg);
    void reset();

    void tick();

    void setRenderer(SDL_Renderer* renderer);
    bool isFrameReady() const;
    void renderFrame();

private:
    uint8_t registers[64];

    uint16_t scanline;
    uint16_t cycle;

    SDL_Renderer* renderer;
    SDL_Texture* texture;
    std::vector<uint32_t> framebuffer;
    bool frameReady;
};
