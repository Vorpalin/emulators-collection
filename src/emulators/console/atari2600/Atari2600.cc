#include "emulators/console/atari2600/Atari2600.hh"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {

// Approximate NTSC palette, generated via YIQ -> RGB (0xAARRGGBB).
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

    pal[static_cast<std::size_t>(c)] = 0xFF000000u | (r << 16) | (g << 8) | b;
  }
  return pal;
}

const std::array<uint32_t, 128> kPalette = buildPalette();

}  // namespace

inline void to_json(json& j, const Atari2600State& state) {
  j = {
      {"bus", state.bus},
      {"keys", state.keys},
      {"sampleRate", state.sampleRate},
      {"sampleAcc", state.sampleAcc},
      {"audio", state.audio},
  };
}

inline void from_json(const json& j, Atari2600State& state) {
  j.at("bus").get_to(state.bus);
  j.at("keys").get_to(state.keys);
  j.at("sampleRate").get_to(state.sampleRate);
  j.at("sampleAcc").get_to(state.sampleAcc);
  j.at("audio").get_to(state.audio);
}

Atari2600::Atari2600()
    : bus(),
      audio(),
      framebuffer_(static_cast<std::size_t>(TIA1A::kWidth) * TIA1A::kHeight *
                   4),
      mono_(),
      audio_() {
  audio_.reserve(4096);
  // Route TIA audio register writes to the sound generator
  bus.setAudioWriteHook(
      [this](uint16_t reg, uint8_t value) { audio.write(reg, value); });
  this->convertFrame();
}

bool Atari2600::loadProgram(const uint8_t* data, std::size_t size) {
  if (!bus.loadROM(data, size)) return false;
  this->reset();
  return true;
}

void Atari2600::reset() {
  bus.reset();
  audio.reset();  // silence both channels
  sampleAcc_ = 0.0;
  audio_.clear();
  for (bool& k : keys_) k = false;
  this->updateInput();
  this->convertFrame();
}

void Atari2600::setAudioSampleRate(int hz) {
  if (hz <= 0) return;
  sampleRate_ = static_cast<double>(hz);
  audio.setSampleRate(hz);
}

void Atari2600::setKey(int key, bool pressed) {
  if (key < 0 || key >= static_cast<int>(sizeof(keys_))) return;
  keys_[key] = pressed;
  this->updateInput();
}

void Atari2600::updateInput() {
  uint8_t swcha = 0xFF;  // 1 = not pressed
  if (keys_[kRight]) swcha &= ~0x80;
  if (keys_[kLeft]) swcha &= ~0x40;
  if (keys_[kDown]) swcha &= ~0x20;
  if (keys_[kUp]) swcha &= ~0x10;

  uint8_t swchb = 0xFF;
  if (keys_[kReset]) swchb &= ~0x01;   // Reset
  if (keys_[kSelect]) swchb &= ~0x02;  // Select

  bus.setInput(swcha, swchb, keys_[kFire], false);
}

void Atari2600::convertFrame() {
  const uint8_t* frame = bus.frame();
  const std::size_t pixels =
      static_cast<std::size_t>(TIA1A::kWidth) * TIA1A::kHeight;
  for (std::size_t i = 0; i < pixels; ++i) {
    const uint32_t argb = kPalette[(frame[i] >> 1) & 0x7F];
    uint8_t* out = &framebuffer_[i * 4];
    out[0] = static_cast<uint8_t>((argb >> 16) & 0xFF);  // R
    out[1] = static_cast<uint8_t>((argb >> 8) & 0xFF);   // G
    out[2] = static_cast<uint8_t>(argb & 0xFF);          // B
    out[3] = 255;                                        // A
  }
}

void Atari2600::generateAudio() {
  audio_.clear();

  // One video frame lasts 1/59.92 s; carry the fractional part over.
  sampleAcc_ += sampleRate_ / kFrameRate;
  const int frames = static_cast<int>(sampleAcc_);
  sampleAcc_ -= frames;
  if (frames <= 0) return;

  mono_.resize(static_cast<std::size_t>(frames));
  audio.generate(mono_.data(), frames);

  audio_.reserve(mono_.size() * 2);
  for (const float s : mono_) {
    audio_.push_back(s);  // left
    audio_.push_back(s);  // right
  }
}

void Atari2600::stepFrame() {
  // Run until the TIA finishes a frame (VSYNC), with a safety cap so a ROM
  // that never syncs cannot freeze the host.
  for (int i = 0; i < kMaxTicksPerFrame && !bus.frameReady(); ++i) {
    bus.tick();
  }
  if (bus.frameReady()) bus.clearFrameReady();

  this->convertFrame();
  this->generateAudio();
}

Atari2600State Atari2600::getState() const {
  return {
      .bus = bus.getState(),
      .keys =
          {
              keys_[0],
              keys_[1],
              keys_[2],
              keys_[3],
              keys_[4],
              keys_[5],
              keys_[6],
          },
      .sampleRate = sampleRate_,
      .sampleAcc = sampleAcc_,
      .audio = audio.getState(),
  };
}

void Atari2600::setState(const Atari2600State& state) {
  bus.setState(state.bus);

  for (std::size_t i = 0; i < state.keys.size(); ++i) {
    keys_[i] = state.keys[i];
  }

  sampleRate_ = state.sampleRate;
  sampleAcc_ = state.sampleAcc;

  audio.setState(state.audio);

  updateInput();

  convertFrame();

  audio_.clear();
  mono_.clear();
}

void saveState(const std::string& path) {
  const Atari2600State state = getState();
  nlohmann::json json = state;

  std::ofstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open save state file");
  }

  file << json.dump(4);
}

void loadState(const std::string& path) {
  std::ifstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open load state file");
  }
  json j;
  file >> j;
  this->setState(j.get<Atari2600State>());
}
