#include "emulators/audio/TIA1A/TIAAudio.hh"

void TIAAudio::reset() {
  for (auto& r : regs_) r = 0;
  ch_[0] = Channel{};
  ch_[1] = Channel{};
  clockAcc_ = 0.0;
  xPrev_ = 0.0f;
  yPrev_ = 0.0f;
}

void TIAAudio::setSampleRate(int hz) {
  if (hz > 0) sampleRate_ = static_cast<double>(hz);
}

void TIAAudio::write(uint16_t addr, uint8_t value) {
  addr &= 0x3F;
  if (addr < 0x15 || addr > 0x1A) return;

  static constexpr uint8_t kMask[6] = {0x0F, 0x0F, 0x1F, 0x1F, 0x0F, 0x0F};
  const int i = addr - 0x15;
  regs_[i] = static_cast<uint8_t>(value & kMask[i]);
}

void TIAAudio::generate(float* out, int count) {
  uint8_t audc[2], audf[2], audv[2];
  for (int c = 0; c < 2; ++c) {
    audc[c] = regs_[0 + c];
    audf[c] = regs_[2 + c];
    audv[c] = regs_[4 + c];
  }

  const double step = kTiaAudioHz / sampleRate_;

  for (int i = 0; i < count; ++i) {
    clockAcc_ += step;
    while (clockAcc_ >= 1.0) {
      clockAcc_ -= 1.0;
      tick(ch_[0], audc[0], audf[0]);
      tick(ch_[1], audc[1], audf[1]);
    }

    const int mix = (ch_[0].out ? audv[0] : 0) + (ch_[1].out ? audv[1] : 0);
    const float x = static_cast<float>(mix) / 30.0f * kAmplitude;

    // One-pole high-pass (~35 Hz): removes the DC offset a constant
    // "on" channel would otherwise produce, like the TV's coupling cap.
    const float y = x - xPrev_ + 0.995f * yPrev_;
    xPrev_ = x;
    yPrev_ = y;
    out[i] = y;
  }
}

bool TIAAudio::lfsr4(uint8_t& r) {
  r = static_cast<uint8_t>(((r << 1) | (((r >> 3) ^ (r >> 2)) & 1)) & 0x0F);
  return r & 1;
}

bool TIAAudio::lfsr5(uint8_t& r) {
  r = static_cast<uint8_t>(((r << 1) | (((r >> 4) ^ (r >> 2)) & 1)) & 0x1F);
  return r & 1;
}

bool TIAAudio::lfsr9(uint16_t& r) {
  r = static_cast<uint16_t>(((r << 1) | (((r >> 8) ^ (r >> 4)) & 1)) & 0x1FF);
  return r & 1;
}

bool TIAAudio::pattern31(int pos) { return pos >= 13; }

void TIAAudio::tick(Channel& ch, uint8_t audc, uint8_t audf) {
  if (ch.div < audf) {  // divide by AUDF + 1
    ++ch.div;
    return;
  }
  ch.div = 0;

  switch (audc & 0x0F) {
    case 0:
    case 11:  // constant level
      ch.out = true;
      break;
    case 1:  // 4-bit poly
      ch.out = lfsr4(ch.p4);
      break;
    case 2: {  // 4-bit poly clocked on div-31 edges
      const bool before = pattern31(ch.pos31);
      ch.pos31 = (ch.pos31 + 1) % 31;
      if (pattern31(ch.pos31) != before) ch.out = lfsr4(ch.p4);
      break;
    }
    case 3:  // 4-bit poly gated by 5-bit poly
      if (lfsr5(ch.p5)) ch.out = lfsr4(ch.p4);
      break;
    case 4:
    case 5:  // pure tone, div 2
      ch.out = !ch.out;
      break;
    case 6:
    case 10:  // div 31 square
      ch.pos31 = (ch.pos31 + 1) % 31;
      ch.out = pattern31(ch.pos31);
      break;
    case 7:
    case 9:  // 5-bit poly
      ch.out = lfsr5(ch.p5);
      break;
    case 8:  // 9-bit poly (white noise)
      ch.out = lfsr9(ch.p9);
      break;
    case 12:
    case 13:  // div 6 square
      ch.pos6 = (ch.pos6 + 1) % 6;
      ch.out = ch.pos6 < 3;
      break;
    case 14:  // div 93 square
      if (++ch.sub >= 3) {
        ch.sub = 0;
        ch.pos31 = (ch.pos31 + 1) % 31;
      }
      ch.out = pattern31(ch.pos31);
      break;
    case 15:  // 5-bit poly at a third of the rate
      if (++ch.sub >= 3) {
        ch.sub = 0;
        ch.out = lfsr5(ch.p5);
      }
      break;
  }
}
