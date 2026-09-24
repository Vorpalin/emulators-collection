#include "Atari2600.hh"

#include <SDL2/SDL.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>

namespace {

// Approximate NTSC palette, generated via YIQ -> RGB.
std::array<uint32_t, 128> buildPalette() {
  std::array<uint32_t, 128> pal{};
  const double kPi = 3.14159265358979323846;

  for (int c = 0; c < 128; ++c) {
    const int hue = c >> 3;  // color byte bits 7-4 (index is byte >> 1)
    const int lum = c & 7;   // color byte bits 3-1

    const double y = 0.08 + 0.92 * (lum / 7.0);
    double i = 0.0, q = 0.0;

    if (hue != 0) {
      const double angle = (-40.0 + (hue - 1) * 24.0) * kPi / 180.0;
      const double sat = 0.28;
      i = sat * std::cos(angle);
      q = sat * std::sin(angle);
    }

    auto clamp = [](double v) {
      return static_cast<uint32_t>(std::fmin(std::fmax(v, 0.0), 1.0) * 255.0 +
                                   0.5);
    };

    const uint32_t r = clamp(y + 0.956 * i + 0.621 * q);
    const uint32_t g = clamp(y - 0.272 * i - 0.647 * q);
    const uint32_t b = clamp(y - 1.106 * i + 1.703 * q);

    pal[c] = 0xFF000000u | (r << 16) | (g << 8) | b;
  }
  return pal;
}

const std::array<uint32_t, 128> kPalette = buildPalette();

}  // namespace

Atari2600::Atari2600()
    : bus(),
      renderer(nullptr),
      frameTexture(nullptr),
      audio(),
      isRunning(true) {
  // Route TIA audio register writes to the sound generator
  bus.setAudioWriteHook(
      [this](uint16_t reg, uint8_t value) { audio.write(reg, value); });
}

Atari2600::~Atari2600() {
  if (frameTexture) SDL_DestroyTexture(frameTexture);
}

void Atari2600::loadProgram(const std::string& filename) {
  bus.loadROM(const_cast<std::string&>(filename));
  bus.reset();
}

void Atari2600::setRenderer(SDL_Renderer* renderer) {
  this->renderer = renderer;
}

void Atari2600::renderFrame() {
  if (!renderer) return;

  if (!frameTexture) {
    frameTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, TIA1A::kWidth,
                                     TIA1A::kHeight);
    if (!frameTexture) return;

    SDL_SetTextureScaleMode(frameTexture, SDL_ScaleModeNearest);
    SDL_RenderSetLogicalSize(renderer, TIA1A::kWidth, TIA1A::kHeight);
  }

  std::vector<uint32_t> pixels(TIA1A::kWidth * TIA1A::kHeight);
  const uint8_t* frame = bus.frame();
  for (size_t i = 0; i < pixels.size(); ++i) {
    pixels[i] = kPalette[(frame[i] >> 1) & 0x7F];
  }

  SDL_UpdateTexture(frameTexture, nullptr, pixels.data(),
                    TIA1A::kWidth * static_cast<int>(sizeof(uint32_t)));
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, frameTexture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

void Atari2600::reset() {
  bus.reset();
  audio.write(0x19, 0);  // silence both channels
  audio.write(0x1A, 0);
  isRunning = true;  // Set the running state to true after reset
}

int Atari2600::run() {
  isRunning =
      true;  // Ensure the running state is true at the start of the run loop

  audio.open();

  // Pace emulation to the NTSC frame rate (~59.92 Hz). The sound is generated
  // in real time, so running faster than that would speed up the audio too.
  const Uint64 perfFreq = SDL_GetPerformanceFrequency();
  const Uint64 framePeriod = static_cast<Uint64>(perfFreq / 59.92);
  Uint64 nextFrame = SDL_GetPerformanceCounter();

  while (isRunning) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT ||
          (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)) {
        isRunning = false;
      }

      const Uint8* keys = SDL_GetKeyboardState(nullptr);

      uint8_t swcha = 0xFF;  // 1 = not pressed
      if (keys[SDL_SCANCODE_RIGHT]) swcha &= ~0x80;
      if (keys[SDL_SCANCODE_LEFT]) swcha &= ~0x40;
      if (keys[SDL_SCANCODE_DOWN]) swcha &= ~0x20;
      if (keys[SDL_SCANCODE_UP]) swcha &= ~0x10;

      uint8_t swchb = 0xFF;
      if (keys[SDL_SCANCODE_F1]) swchb &= ~0x01;   // Reset
      if (keys[SDL_SCANCODE_TAB]) swchb &= ~0x02;  // Select

      const bool fire0 = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_Z];

      bus.setInput(swcha, swchb, fire0, false);
    }

    for (int cycle = 0; cycle < 10000 && isRunning && !bus.frameReady();
         ++cycle) {
      bus.tick();
    }

    if (bus.frameReady()) {
      renderFrame();
      bus.clearFrameReady();
      audio.update();

      nextFrame += framePeriod;
      const Uint64 now = SDL_GetPerformanceCounter();
      if (nextFrame > now) {
        SDL_Delay(static_cast<Uint32>((nextFrame - now) * 1000 / perfFreq));
      } else {
        nextFrame = now;  // fell behind: don't try to catch up in a burst
      }
    }
  }

  audio.close();  // close while SDL is still initialised
  return 0;
}
