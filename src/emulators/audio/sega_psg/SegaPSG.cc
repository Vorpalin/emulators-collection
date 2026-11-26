#include "emulators/audio/sega_psg/SegaPSG.hh"

#include <cmath>

SegaPSG::SegaPSG() {
  // 2 dB per attenuation step; four channels at full volume sum to 1.0.
  for (size_t i = 0; i < 15; i++)
    amplitude_[i] = static_cast<float>(0.25 * std::pow(10.0, -0.1 * static_cast<double>(i)));
  amplitude_[15] = 0.0f;
  reset();
  updateRate();
}

void SegaPSG::reset() {
  state_ = SegaPSGState{};
  samples_.clear();
}

void SegaPSG::setSampleRate(int hz) {
  sampleRate_ = hz > 0 ? hz : 48000;
  updateRate();
}

void SegaPSG::updateRate() {
  ticksPerSample_ = (clockHz_ / kCyclesPerTick) / sampleRate_;
  // One-pole high-pass at roughly 20 Hz removes the DC offset of the
  // unipolar square waves.
  const double pi = 3.14159265358979323846;
  hpAlpha_ = 1.0 - 2.0 * pi * 20.0 / sampleRate_;
}

void SegaPSG::write(uint8_t value) {
  if (value & 0x80) {
    state_.latch = (value >> 4) & 0x07;
    const int ch = state_.latch >> 1;
    const bool isVolume = (state_.latch & 1) != 0;
    const uint8_t data = value & 0x0F;
    if (isVolume) {
      state_.volume[static_cast<size_t>(ch)] = data;
    } else if (ch < 3) {
      auto &p = state_.tonePeriod[static_cast<size_t>(ch)];
      p = static_cast<uint16_t>((p & 0x3F0) | data);
    } else {
      state_.noiseControl = data & 0x07;
      state_.lfsr = 0x8000;
    }
    return;
  }

  // Data byte: updates the latched register.
  const int ch = state_.latch >> 1;
  const bool isVolume = (state_.latch & 1) != 0;
  if (isVolume) {
    state_.volume[static_cast<size_t>(ch)] = value & 0x0F;
  } else if (ch < 3) {
    auto &p = state_.tonePeriod[static_cast<size_t>(ch)];
    p = static_cast<uint16_t>(((value & 0x3F) << 4) | (p & 0x00F));
  } else {
    state_.noiseControl = value & 0x07;
    state_.lfsr = 0x8000;
  }
}

bool SegaPSG::channelOutput(int ch) const {
  if (ch < 3) {
    if (state_.tonePeriod[static_cast<size_t>(ch)] < 2) return true;
    return state_.flip[static_cast<size_t>(ch)];
  }
  return (state_.lfsr & 1) != 0;
}

void SegaPSG::shiftNoise() {
  int feedback;
  if (state_.noiseControl & 0x04) {  // White noise: taps on bits 0 and 3.
    uint16_t t = state_.lfsr & 0x0009;
    feedback = ((t ^ (t >> 3)) & 1);
  } else {  // Periodic noise.
    feedback = state_.lfsr & 1;
  }
  state_.lfsr = static_cast<uint16_t>((state_.lfsr >> 1) | (feedback << 15));
}

void SegaPSG::tick() {
  for (size_t ch = 0; ch < 3; ch++) {
    if (--state_.counter[ch] <= 0) {
      state_.counter[ch] = state_.tonePeriod[ch] > 0 ? state_.tonePeriod[ch] : 1;
      state_.flip[ch] = !state_.flip[ch];
    }
  }
  if (--state_.counter[3] <= 0) {
    const int rate = state_.noiseControl & 3;
    int reload = rate == 3 ? state_.tonePeriod[2] : (0x10 << rate);
    if (reload < 1) reload = 1;
    state_.counter[3] = reload;
    state_.flip[3] = !state_.flip[3];
    if (state_.flip[3]) shiftNoise();
  }

  double l = 0.0, r = 0.0;
  for (int ch = 0; ch < 4; ch++) {
    if (!channelOutput(ch)) continue;
    const double a = amplitude_[state_.volume[static_cast<size_t>(ch)]];
    if (state_.stereo & (0x10 << ch)) l += a;
    if (state_.stereo & (0x01 << ch)) r += a;
  }
  emit(l, r);
}

void SegaPSG::emit(double l, double r) {
  state_.accL += l;
  state_.accR += r;
  state_.accCount += 1.0;
  state_.phase += 1.0;
  if (state_.phase < ticksPerSample_) return;
  state_.phase -= ticksPerSample_;

  double outL = state_.accL / state_.accCount;
  double outR = state_.accR / state_.accCount;
  state_.accL = state_.accR = state_.accCount = 0.0;

  if (highPass_) {
    const double fl = outL - state_.hpPrevInL + hpAlpha_ * state_.hpPrevOutL;
    const double fr = outR - state_.hpPrevInR + hpAlpha_ * state_.hpPrevOutR;
    state_.hpPrevInL = outL;
    state_.hpPrevInR = outR;
    state_.hpPrevOutL = fl;
    state_.hpPrevOutR = fr;
    outL = fl;
    outR = fr;
  }
  samples_.push_back(static_cast<float>(outL));
  samples_.push_back(static_cast<float>(outR));
}

void SegaPSG::step(uint32_t cpuCycles) {
  state_.cycleRemainder += static_cast<int>(cpuCycles);
  while (state_.cycleRemainder >= kCyclesPerTick) {
    state_.cycleRemainder -= kCyclesPerTick;
    tick();
  }
}
