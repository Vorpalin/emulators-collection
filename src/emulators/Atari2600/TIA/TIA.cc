#include "TIA.hh"

#include <algorithm>
#include <cmath>

namespace
{
constexpr int kHblankCycles = 68;

uint32_t convertTIAColor(uint8_t tiaColor)
{
    const uint8_t hue = (tiaColor >> 4) & 0x0F; // bits D4-D7
    const uint8_t lum = (tiaColor >> 1) & 0x07; // bits D1-D3 (D0 ignoré)

    constexpr uint8_t kLumaTable[8] = {0x00, 0x40, 0x6C, 0x90, 0xB0, 0xC8, 0xDC, 0xEC};
    const double y = kLumaTable[lum] / 255.0;

    double r = y;
    double g = y;
    double b = y;

    if (hue != 0) {
        const double angleDeg = -(static_cast<double>(hue) - 1.0) * (360.0 / 15.0) - 15.0;
        const double angleRad = angleDeg * M_PI / 180.0;
        const double chromaAmplitude = 0.30;
        const double i = chromaAmplitude * std::cos(angleRad);
        const double q = chromaAmplitude * std::sin(angleRad);

        r = y + 0.956 * i + 0.621 * q;
        g = y - 0.272 * i - 0.647 * q;
        b = y - 1.105 * i + 1.702 * q;
    }

    const auto clamp8 = [](double v) -> uint8_t {
        return static_cast<uint8_t>(std::clamp(v, 0.0, 1.0) * 255.0 + 0.5);
    };

    return (0xFFu << 24) | (static_cast<uint32_t>(clamp8(r)) << 16)
           | (static_cast<uint32_t>(clamp8(g)) << 8) | static_cast<uint32_t>(clamp8(b));
}

bool getPlayfieldBit(uint8_t pf0, uint8_t pf1, uint8_t pf2, int index)
{
    if (index < 4) {
        return ((pf0 >> (4 + index)) & 0x01) != 0;
    }
    if (index < 12) {
        const int bit = 7 - (index - 4);
        return ((pf1 >> bit) & 0x01) != 0;
    }
    const int bit = index - 12;
    return ((pf2 >> bit) & 0x01) != 0;
}
}

TIA::TIA()
  : scanline(0)
  , cycle(0)
  , renderer(nullptr)
  , texture(nullptr)
  , framebuffer(ScreenWidth * ScreenHeight, 0xFF000000)
  , frameReady(false)
{
    this->reset();
}

TIA::~TIA()
{
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
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
    if (cycle >= kHblankCycles && scanline < ScreenHeight) {
        const int x = cycle - kHblankCycles;

        const uint8_t pf0 = registers[0x0D];
        const uint8_t pf1 = registers[0x0E];
        const uint8_t pf2 = registers[0x0F];
        const uint8_t ctrlpf = registers[0x0A];
        const bool reflect = (ctrlpf & 0x01) != 0;

        int pfIndex;
        if (x < ScreenWidth / 2) {
            pfIndex = x / 4;
        } else {
            const int rx = x - ScreenWidth / 2;
            pfIndex = reflect ? (19 - rx / 4) : (rx / 4);
        }

        const bool pfBit = getPlayfieldBit(pf0, pf1, pf2, pfIndex);
        const uint8_t colorReg = pfBit ? registers[0x08] /* COLUPF */ : registers[0x09] /* COLUBK */;

        framebuffer[scanline * ScreenWidth + x] = convertTIAColor(colorReg);
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
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
        texture = nullptr;
    }

    this->renderer = renderer;
    if (this->renderer != nullptr) {
        SDL_RenderSetLogicalSize(this->renderer, ScreenWidth, ScreenHeight);
        texture = SDL_CreateTexture(this->renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, ScreenWidth, ScreenHeight);
    }
}

bool TIA::isFrameReady() const
{
    return frameReady;
}

void TIA::renderFrame()
{
    if (renderer == nullptr || texture == nullptr || !frameReady) {
        return;
    }

    SDL_UpdateTexture(texture, nullptr, framebuffer.data(),
                       ScreenWidth * static_cast<int>(sizeof(uint32_t)));

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);

    frameReady = false;
}