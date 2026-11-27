#include "emulators/console/master_system/MasterSystem.hh"

#include <iostream>
#include <nlohmann/json.hpp>

namespace {
// Expands a 5-bit channel to 8 bits.
inline uint8_t expand5(unsigned v) {
  return static_cast<uint8_t>((v << 3) | (v >> 2));
}
}  // namespace

MasterSystem::MasterSystem(SegaVDP::Model model) : bus_(model) {
  bus_.psg().setSampleRate(sampleRate_);
  for (std::size_t i = 3; i < framebuffer_.size(); i += 4) framebuffer_[i] = 0xFF;
}

bool MasterSystem::loadProgram(const uint8_t* data, std::size_t size) {
  loaded_ = bus_.loadROM(data, size);
  if (!loaded_) {
    std::cerr << "Master System: " << bus_.error() << std::endl;
    return false;
  }
  audio_.clear();
  renderFrame();
  return true;
}

void MasterSystem::reset() {
  if (!loaded_) return;
  bus_.reset();
  audio_.clear();
  renderFrame();
}

void MasterSystem::setAudioSampleRate(int hz) {
  sampleRate_ = hz;
  bus_.psg().setSampleRate(hz);
}

void MasterSystem::setKey(int key, bool pressed) {
  switch (key) {
    case kRight: bus_.setButton(0, MasterSystemBus::kRight, pressed); break;
    case kLeft: bus_.setButton(0, MasterSystemBus::kLeft, pressed); break;
    case kUp: bus_.setButton(0, MasterSystemBus::kUp, pressed); break;
    case kDown: bus_.setButton(0, MasterSystemBus::kDown, pressed); break;
    case kA: bus_.setButton(0, MasterSystemBus::kButton1, pressed); break;
    case kB: bus_.setButton(0, MasterSystemBus::kButton2, pressed); break;
    case kStart:
      // The Game Gear has a real Start button; the Master System only has
      // Pause, which raises a non-maskable interrupt.
      if (bus_.isGameGear()) {
        bus_.setStart(pressed);
      } else {
        bus_.setPause(pressed);
      }
      break;
    default: break;  // Select and unknown keys do nothing.
  }
}

void MasterSystem::stepFrame() {
  audio_.clear();
  if (!loaded_) return;
  bus_.psg().clearSamples();
  // A frame is ~59,700 CPU cycles (~15,000 instructions); the bound only
  // protects against a stuck VDP and never triggers in normal operation.
  for (int guard = 0; guard < 400000; ++guard) {
    bus_.step();
    if (bus_.consumeFrameReady()) break;
  }
  audio_ = bus_.psg().samples();  // Already interleaved stereo.
  renderFrame();
}

void MasterSystem::renderFrame() {
  const auto& src = bus_.vdp().framebuffer();
  const int x0 = bus_.vdp().viewportX();
  const int y0 = bus_.vdp().viewportY();
  const int w = bus_.vdp().viewportWidth();
  const int h = bus_.vdp().viewportHeight();
  std::size_t out = 0;
  for (int y = 0; y < h; ++y) {
    const std::size_t row = static_cast<std::size_t>(y0 + y) * SegaVDP::kWidth;
    for (int x = 0; x < w; ++x) {
      const uint16_t c = src[row + static_cast<std::size_t>(x0 + x)];
      framebuffer_[out++] = expand5(c & 0x1F);
      framebuffer_[out++] = expand5((c >> 5) & 0x1F);
      framebuffer_[out++] = expand5((c >> 10) & 0x1F);
      framebuffer_[out++] = 0xFF;
    }
  }
}

std::string MasterSystem::saveState() const {
  const nlohmann::json j = bus_.getState();
  return j.dump();
}

bool MasterSystem::loadState(const std::string& json) {
  if (!loaded_) return false;
  try {
    const nlohmann::json j = nlohmann::json::parse(json);
    bus_.setState(j.get<MasterSystemBusState>());
    renderFrame();
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Failed to parse JSON for loading state: " << e.what()
              << std::endl;
    return false;
  }
}
