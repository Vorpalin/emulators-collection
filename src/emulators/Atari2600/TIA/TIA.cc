#include "TIA.hh"

#include <algorithm>

namespace
{
uint32_t convertTIAColor(uint8_t tiaColor)
{
    const uint8_t intensity = static_cast<uint8_t>((tiaColor & 0x0F) * 17);
    const uint8_t hue = static_cast<uint8_t>((tiaColor >> 4) & 0x0F);

    const uint8_t r = static_cast<uint8_t>(std::min<int>(255, intensity + hue * 8));
    const uint8_t g = static_cast<uint8_t>(std::max<int>(0, intensity - hue * 4));
    const uint8_t b = static_cast<uint8_t>(std::min<int>(255, intensity + (15 - hue) * 6));

    return (0xFFu << 24) | (static_cast<uint32_t>(r) << 16)
           | (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
}
}

TIA::TIA()
  : scanline(0)
  , cycle(0)
  , renderer(nullptr)
  , framebuffer(ScreenWidth * ScreenHeight, 0xFF000000)
  , frameReady(false)
{
    this->reset();
}

void TIA::reset()
{
    for (auto& reg : registers) {
        reg = 0;
    }

    scanline = 0;
    cycle = 0;
    frameReady = false;
    std::fill(framebuffer.begin(), framebuffer.end(), 0xFF000000);
}

void TIA::write(uint8_t reg, uint8_t value)
{
    if (reg < sizeof(registers)) {
        registers[reg] = value;
    }
}

uint8_t TIA::read(uint8_t reg)
{
    if (reg < sizeof(registers)) {
        return registers[reg];
    }
    return 0xFF;
}

void TIA::tick()
{
    if (scanline < ScreenHeight && cycle < ScreenWidth) {
        const uint8_t bgColorReg = registers[0x09];
        framebuffer[scanline * ScreenWidth + cycle] = convertTIAColor(bgColorReg);
    }

    ++cycle;
    if (cycle >= 228) {
        cycle = 0;
        ++scanline;
    }

    if (scanline >= 262) {
        scanline = 0;
        frameReady = true;
    }
}

void TIA::setRenderer(SDL_Renderer* renderer)
{
    this->renderer = renderer;
    if (this->renderer != nullptr) {
        SDL_RenderSetLogicalSize(this->renderer, ScreenWidth, ScreenHeight);
    }
}

bool TIA::isFrameReady() const
{
    return frameReady;
}

void TIA::renderFrame()
{
    if (renderer == nullptr || !frameReady) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (int y = 0; y < ScreenHeight; ++y) {
        for (int x = 0; x < ScreenWidth; ++x) {
            const uint32_t color = framebuffer[y * ScreenWidth + x];
            const uint8_t r = static_cast<uint8_t>((color >> 16) & 0xFF);
            const uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
            const uint8_t b = static_cast<uint8_t>(color & 0xFF);

            SDL_SetRenderDrawColor(renderer, r, g, b, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);
    frameReady = false;
}
