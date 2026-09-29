#include "emulators/audio/TIA1A/TIAAudio.hh"

TIAAudio::TIAAudio() {
  for (auto& r : regs_) r.store(0);
}

bool TIAAudio::open(int freq, int bufferSamples) {
  if (dev_ != 0) return true;
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    std::cerr << "Audio init failed: " << SDL_GetError() << '\n';
    return false;
  }
  audioInit_ = true;

  SDL_AudioSpec want{}, have{};
  want.freq = freq;
  want.format = AUDIO_S16SYS;
  want.channels = 1;
  want.samples = static_cast<Uint16>(bufferSamples);
  want.callback = &TIAAudio::callback;
  want.userdata = this;

  dev_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have,
                             SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (dev_ == 0) {
    std::cerr << "Could not open audio device: " << SDL_GetError() << '\n';
    close();  // undo SDL_InitSubSystem
    return false;
  }

  sampleRate_ = have.freq;

  // Start corked; the first non-zero AUDV write starts the stream.
  SDL_PauseAudioDevice(dev_, 1);
  paused_ = true;
  return true;
}

void TIAAudio::close() {
  if (dev_ != 0) {
    SDL_PauseAudioDevice(dev_, 1);  // make sure the callback is idle
    SDL_CloseAudioDevice(dev_);     // waits for the audio thread to exit
    dev_ = 0;
  }
  paused_ = true;
  if (audioInit_) {
    if (SDL_WasInit(SDL_INIT_AUDIO)) SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioInit_ = false;
  }
}

void TIAAudio::write(uint16_t addr, uint8_t value) {
  addr &= 0x3F;
  if (addr < 0x15 || addr > 0x1A) return;

  static constexpr uint8_t kMask[6] = {0x0F, 0x0F, 0x1F, 0x1F, 0x0F, 0x0F};
  const int i = addr - 0x15;
  regs_[i].store(value & kMask[i], std::memory_order_relaxed);

  // Wake the device as soon as something audible is requested.
  if ((i == 4 || i == 5) && (value & 0x0F) && paused_ && dev_ != 0) {
    SDL_PauseAudioDevice(dev_, 0);
    paused_ = false;
  }
  if (i == 4 || i == 5) idleFrames_ = 0;
}

void TIAAudio::update() {
  if (dev_ == 0 || paused_) return;
  const bool silent = (regs_[4].load(std::memory_order_relaxed) == 0) &&
                      (regs_[5].load(std::memory_order_relaxed) == 0);
  if (!silent) {
    idleFrames_ = 0;
    return;
  }
  if (++idleFrames_ > 30) {
    SDL_PauseAudioDevice(dev_, 1);
    paused_ = true;
  }
}

void TIAAudio::setSampleRate(int hz) { sampleRate_ = hz; }

void TIAAudio::generate(int16_t* out, int count) {
  uint8_t audc[2], audf[2], audv[2];
  for (int c = 0; c < 2; ++c) {
    audc[c] = regs_[0 + c].load(std::memory_order_relaxed);
    audf[c] = regs_[2 + c].load(std::memory_order_relaxed);
    audv[c] = regs_[4 + c].load(std::memory_order_relaxed);
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
    out[i] = static_cast<int16_t>(y);
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

void TIAAudio::callback(void* userdata, Uint8* stream, int len) {
  static_cast<TIAAudio*>(userdata)->generate(
      reinterpret_cast<int16_t*>(stream),
      len / static_cast<int>(sizeof(int16_t)));
}
