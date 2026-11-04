#include "emulators/console/nes/Nes.hh"

#include <iostream>
#include <nlohmann/json.hpp>

namespace {

// "Smooth (FBX)" 2C02 palette, 0xRRGGBB.
constexpr uint32_t kBasePalette[64] = {
    0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600,
    0x561D00, 0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000,
    0x000000, 0x000000, 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC,
    0xB71E7B, 0xB53120, 0x994E00, 0x6B6D00, 0x388700, 0x0C9300, 0x008F32,
    0x007C8D, 0x000000, 0x000000, 0x000000, 0xFFFEFF, 0x64B0FF, 0x9290FF,
    0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22, 0xBCBE00, 0x88D800,
    0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000, 0xFFFEFF,
    0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
    0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000,
    0x000000};

constexpr double kEmphasisAttenuation = 0.75;

}  // namespace

Nes::Nes() {
  // Emphasis bits (PPUMASK bits 5-7) darken the two other colour channels.
  for (std::size_t emph = 0; emph < 8; ++emph) {
    for (std::size_t i = 0; i < 64; ++i) {
      double r = (kBasePalette[i] >> 16) & 0xFF;
      double g = (kBasePalette[i] >> 8) & 0xFF;
      double b = kBasePalette[i] & 0xFF;
      if (emph & 1) g *= kEmphasisAttenuation, b *= kEmphasisAttenuation;
      if (emph & 2) r *= kEmphasisAttenuation, b *= kEmphasisAttenuation;
      if (emph & 4) r *= kEmphasisAttenuation, g *= kEmphasisAttenuation;
      palette_[emph * 64 + i] = (static_cast<uint32_t>(r) << 16) |
                                (static_cast<uint32_t>(g) << 8) |
                                static_cast<uint32_t>(b);
    }
  }
  bus_.apu().setSampleRate(48000);
  for (std::size_t i = 3; i < framebuffer_.size(); i += 4)
    framebuffer_[i] = 0xFF;
}

bool Nes::loadProgram(const uint8_t* data, std::size_t size) {
  loaded_ = bus_.loadROM(data, size);
  if (!loaded_) {
    std::cerr << "NES: " << bus_.error() << std::endl;
    return false;
  }
  audio_.clear();
  renderFrame();
  return true;
}

void Nes::reset() {
  if (!loaded_) return;
  bus_.reset();
  audio_.clear();
}

void Nes::setAudioSampleRate(int hz) { bus_.apu().setSampleRate(hz); }

void Nes::setKey(int key, bool pressed) {
  NesController::Button b;
  switch (key) {
    case kRight:
      b = NesController::kRight;
      break;
    case kLeft:
      b = NesController::kLeft;
      break;
    case kUp:
      b = NesController::kUp;
      break;
    case kDown:
      b = NesController::kDown;
      break;
    case kA:
      b = NesController::kA;
      break;
    case kB:
      b = NesController::kB;
      break;
    case kSelect:
      b = NesController::kSelect;
      break;
    case kStart:
      b = NesController::kStart;
      break;
    default:
      return;
  }
  bus_.pad(0).setButton(b, pressed);
}

void Nes::stepFrame() {
  audio_.clear();
  if (!loaded_) return;

  bus_.apu().clearSamples();
  // A frame is ~29780 CPU cycles; the bound only protects against a stuck
  // PPU (it can never trigger in normal operation).
  for (int guard = 0; guard < 200000; ++guard) {
    bus_.step();
    if (bus_.consumeFrameReady()) break;
  }

  const std::vector<float>& mono = bus_.apu().samples();
  audio_.reserve(mono.size() * 2);
  for (float s : mono) {
    audio_.push_back(s);
    audio_.push_back(s);
  }
  renderFrame();
}

void Nes::renderFrame() {
  const auto& src = bus_.ppu().frameBuffer();
  for (std::size_t i = 0; i < src.size(); ++i) {
    const uint32_t c = palette_[src[i] & 0x1FF];
    framebuffer_[i * 4 + 0] = static_cast<uint8_t>(c >> 16);
    framebuffer_[i * 4 + 1] = static_cast<uint8_t>(c >> 8);
    framebuffer_[i * 4 + 2] = static_cast<uint8_t>(c);
    framebuffer_[i * 4 + 3] = 0xFF;
  }
}

std::string Nes::saveState() const {
  const nlohmann::json j = bus_.getState();
  return j.dump();
}

bool Nes::loadState(const std::string& json) {
  if (!loaded_) return false;
  try {
    const nlohmann::json j = nlohmann::json::parse(json);
    bus_.setState(j.get<NesBusState>());
    renderFrame();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse JSON for loading state: " << e.what()
              << std::endl;
    return false;
  }
}
