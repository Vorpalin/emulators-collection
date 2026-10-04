#include "emulators/console/game_boy/GameBoy.hh"

namespace {

// Same grey shades as the former SDL renderer (R = G = B).
constexpr uint8_t kShades[4] = {0xFF, 0xAA, 0x55, 0x00};

}  // namespace

GameBoyState GameBoy::getState() const {
  return {
      .bus = bus.getState(),
      .cyclesPerSample = cyclesPerSample_,
      .phase = phase_,
      .sumL = sumL_,
      .sumR = sumR_,
      .weight = weight_,
  };
}

void GameBoy::setState(const GameBoyState& state) {
  bus.setState(state.bus);

  cyclesPerSample_ = state.cyclesPerSample;
  phase_ = state.phase;
  sumL_ = state.sumL;
  sumR_ = state.sumR;
  weight_ = state.weight;

  audio_.clear();

  convertFrame();
}

void to_json(nlohmann::json& j, const GameBoyState& state) {
  j = {
      {"bus", state.bus},     {"cyclesPerSample", state.cyclesPerSample},
      {"phase", state.phase}, {"sumL", state.sumL},
      {"sumR", state.sumR},   {"weight", state.weight},
  };
}

void from_json(const nlohmann::json& j, GameBoyState& state) {
  j.at("bus").get_to(state.bus);
  j.at("cyclesPerSample").get_to(state.cyclesPerSample);
  j.at("phase").get_to(state.phase);
  j.at("sumL").get_to(state.sumL);
  j.at("sumR").get_to(state.sumR);
  j.at("weight").get_to(state.weight);
}

GameBoy::GameBoy() : bus() {
  audio_.reserve(4096);
  this->convertFrame();
}

bool GameBoy::loadProgram(const uint8_t* data, std::size_t size) {
  this->reset();
  if (!bus.loadROM(data, size)) return false;
  this->convertFrame();
  return true;
}

void GameBoy::reset() {
  bus.reset();
  phase_ = 0.0;
  sumL_ = sumR_ = weight_ = 0.0;
  audio_.clear();
}

void GameBoy::setAudioSampleRate(int hz) {
  if (hz <= 0) return;
  cyclesPerSample_ = kCpuHz / static_cast<double>(hz);
  phase_ = 0.0;
  sumL_ = sumR_ = weight_ = 0.0;
}

void GameBoy::setKey(int key, bool pressed) {
  if (key < kRight || key > kStart) return;
  bus.getJoypad().setButton(static_cast<GameBoyController::Button>(key),
                            pressed);
}

void GameBoy::convertFrame() {
  const auto& shades = bus.getFramebuffer();
  for (std::size_t i = 0; i < shades.size(); ++i) {
    const uint8_t v = kShades[shades[i] & 0x03];
    uint8_t* out = &framebuffer_[i * 4];
    out[0] = v;
    out[1] = v;
    out[2] = v;
    out[3] = 255;
  }
}

void GameBoy::stepFrame() {
  audio_.clear();

  uint32_t ran = 0;
  while (ran < kMaxCyclesPerStep) {
    const uint32_t cycles = bus.step();
    ran += cycles;

    // Sample the APU after every instruction and average the samples,
    // weighted by the cycles they lasted, down to the output rate.
    float l = 0.0f;
    float r = 0.0f;
    bus.getStereoSample(l, r);
    sumL_ += static_cast<double>(l) * cycles;
    sumR_ += static_cast<double>(r) * cycles;
    weight_ += cycles;

    phase_ += cycles;
    if (phase_ >= cyclesPerSample_) {
      phase_ -= cyclesPerSample_;
      if (weight_ > 0.0) {
        audio_.push_back(static_cast<float>(sumL_ / weight_));
        audio_.push_back(static_cast<float>(sumR_ / weight_));
      }
      sumL_ = sumR_ = weight_ = 0.0;
    }

    if (bus.consumeFrameReady()) break;  // full frame rendered by the PPU
  }

  this->convertFrame();
}

void GameBoy::saveState(const std::string& filename) const {
  nlohmann::json j = this->getState();
  std::ofstream file(filename);
  if (!file)
    throw std::runtime_error("Failed to open file for writing: " + filename);
  file << j.dump(4);
}

void GameBoy::loadState(const std::string& filename) {
  std::ifstream file(filename);
  if (!file)
    throw std::runtime_error("Failed to open file for reading: " + filename);
  nlohmann::json j;
  file >> j;
  this->setState(j.get<GameBoyState>());
}
