#pragma once

// Minimal TIA sound emulation (2 channels) with SDL2 output. Header-only.
//
// Feed it every CPU write to the TIA audio registers with write():
//   0x15 AUDC0   0x16 AUDC1   (waveform / noise type, 4 bits)
//   0x17 AUDF0   0x18 AUDF1   (frequency divider, 5 bits)
//   0x19 AUDV0   0x1A AUDV1   (volume, 4 bits)
//
// The waveform is generated in the SDL audio thread from the latest register
// values, so the emulator never has to queue samples.

#include <SDL2/SDL.h>

#include <atomic>
#include <cstdint>
#include <iostream>

class TIAAudio {
 public:
  // NTSC: 3.579545 MHz colour clock / 114 = ~31.4 kHz audio clock.
  static constexpr double kTiaAudioHz = 3579545.0 / 114.0;
  static constexpr float kAmplitude = 8000.0f;  // out of 32767, keep it quiet

  TIAAudio() {
    std::cerr << "TIA audio: " << kTiaAudioHz << " Hz, "
              << (kTiaAudioHz / 44100.0) << " samples per 44.1 kHz sample\n";
    for (auto& r : regs_) r.store(0);
  }
  ~TIAAudio() { close(); }
  TIAAudio(const TIAAudio&) = delete;
  TIAAudio& operator=(const TIAAudio&) = delete;

  bool open(int freq = 44100, int bufferSamples = 512) {
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
    std::cerr << "Audio: driver=" << SDL_GetCurrentAudioDriver() << ", "
              << have.freq << " Hz, buffer " << have.samples << " samples (~"
              << (1000.0 * have.samples / have.freq) << " ms)\n";

    // Start corked; the first non-zero AUDV write starts the stream.
    SDL_PauseAudioDevice(dev_, 1);
    paused_ = true;
    return true;
  }

  // Safe to call several times, and safe after SDL_Quit(). Call it while SDL
  // is still alive (e.g. at the end of run()) rather than relying on the
  // destructor, which may run after the program has already shut SDL down.
  void close() {
    if (dev_ != 0) {
      std::cerr << "Closing audio device\n";
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

  // Call for every write to the TIA. `addr` may be the full bus address.
  void write(uint16_t addr, uint8_t value) {
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

  // Call once per emulated frame. Corks the stream after ~0.5 s of silence,
  // which also flushes any latency the audio server has accumulated.
  void update() {
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

  void setSampleRate(int hz) { sampleRate_ = hz; }

  // Fill `out` with `count` mono samples. Called from the SDL audio thread.
  void generate(int16_t* out, int count) {
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

 private:
  struct Channel {
    int div = 0;    // AUDF divider counter
    int pos31 = 0;  // position in the 31-step pattern
    int pos6 = 0;   // position in the 6-step pattern
    int sub = 0;    // extra /3 stage for modes 14 and 15
    uint8_t p4 = 0x0F;
    uint8_t p5 = 0x1F;
    uint16_t p9 = 0x1FF;
    bool out = false;
  };

  // Maximal-length LFSRs: x^4+x^3+1 (15), x^5+x^3+1 (31), x^9+x^5+1 (511).
  static bool lfsr4(uint8_t& r) {
    r = static_cast<uint8_t>(((r << 1) | (((r >> 3) ^ (r >> 2)) & 1)) & 0x0F);
    return r & 1;
  }
  static bool lfsr5(uint8_t& r) {
    r = static_cast<uint8_t>(((r << 1) | (((r >> 4) ^ (r >> 2)) & 1)) & 0x1F);
    return r & 1;
  }
  static bool lfsr9(uint16_t& r) {
    r = static_cast<uint16_t>(((r << 1) | (((r >> 8) ^ (r >> 4)) & 1)) & 0x1FF);
    return r & 1;
  }
  static bool pattern31(int pos) { return pos >= 13; }  // 18 high, 13 low

  // One tick of the ~31.4 kHz audio clock for one channel.
  static void tick(Channel& ch, uint8_t audc, uint8_t audf) {
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

  static void callback(void* userdata, Uint8* stream, int len) {
    static_cast<TIAAudio*>(userdata)->generate(
        reinterpret_cast<int16_t*>(stream),
        len / static_cast<int>(sizeof(int16_t)));
  }

  SDL_AudioDeviceID dev_ = 0;
  bool audioInit_ = false;  // we called SDL_InitSubSystem(SDL_INIT_AUDIO)
  bool paused_ = true;
  int idleFrames_ = 0;
  double sampleRate_ = 44100.0;

  // 0 AUDC0, 1 AUDC1, 2 AUDF0, 3 AUDF1, 4 AUDV0, 5 AUDV1
  std::atomic<uint8_t> regs_[6];

  // Audio-thread state only.
  Channel ch_[2];
  double clockAcc_ = 0.0;
  float xPrev_ = 0.0f, yPrev_ = 0.0f;
};
