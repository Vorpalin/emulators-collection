#include "emulators/console/game_boy/GameBoy.hh"

#include <algorithm>
#include <cstdio>

namespace {

constexpr int GB_WIDTH = 160;
constexpr int GB_HEIGHT = 144;

constexpr double kCpuHz = 4194304.0;  // T-cycles per second

// Frames per SDL audio callback (what we ask SDL_OpenAudioDevice for).
constexpr int kDeviceFrames = 1024;

// Emulator/audio sync granularity: one chunk = 512 output samples (~12 ms).
constexpr int kChunkFrames = 512;

// Audio kept queued ahead of the device. SDL pulls a whole device buffer at a
// time, so this must stay comfortably above kDeviceFrames or it underruns.
constexpr int kTargetFrames = 3 * kDeviceFrames;

// If the host stalls for longer than this, stop trying to catch up.
constexpr double kMaxCatchUpSeconds = 0.1;

// Sample-rate nudge (+-1 %) that keeps the audio queue level steady while the
// wall clock drives the emulation speed.
constexpr double kMaxRateAdjust = 0.01;

}  // namespace

GameBoy::GameBoy() : bus() {}

GameBoy::~GameBoy() {
  closeAudio();
  if (texture) {
    SDL_DestroyTexture(texture);
  }
}

void GameBoy::openAudio() {
  if (audioDevice != 0) {
    return;
  }
  if (!audioSubsystem) {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
      return;  // audio is optional: we fall back to clock-based pacing
    }
    audioSubsystem = true;
  }

  SDL_AudioSpec want;
  SDL_zero(want);
  want.freq = 44100;
  want.format = AUDIO_F32SYS;
  want.channels = 2;
  want.samples = kDeviceFrames;
  want.callback = nullptr;  // queue mode: we push samples with SDL_QueueAudio

  SDL_AudioSpec have;
  SDL_zero(have);

  audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have,
                                    SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (audioDevice == 0) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioSubsystem = false;
    return;
  }
  sampleRate = have.freq;
}

void GameBoy::closeAudio() {
  if (audioDevice != 0) {
    SDL_PauseAudioDevice(audioDevice, 1);
    SDL_CloseAudioDevice(audioDevice);
    audioDevice = 0;
  }
  if (audioSubsystem) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    audioSubsystem = false;
  }
}

void GameBoy::loadProgram(const std::string& filename) {
  reset();
  bus.loadROM(const_cast<std::string&>(filename));
  if (texture) {
    SDL_DestroyTexture(texture);
  }
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                              SDL_TEXTUREACCESS_STREAMING, GB_WIDTH, GB_HEIGHT);
}

void GameBoy::reset() {
  bus.reset();
  isRunning = true;
}

void GameBoy::handleInput() {
  SDL_Event event;
  GameBoyController& joypad = bus.getJoypad();
  while (SDL_PollEvent(&event)) {
    if (event.type == SDL_QUIT) {
      isRunning = false;
    } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
      bool pressed = (event.type == SDL_KEYDOWN);
      switch (event.key.keysym.sym) {
        case SDLK_RIGHT:
          joypad.setButton(GameBoyController::Right, pressed);
          break;
        case SDLK_LEFT:
          joypad.setButton(GameBoyController::Left, pressed);
          break;
        case SDLK_ESCAPE:
          isRunning = false;
          break;
        case SDLK_UP:
          joypad.setButton(GameBoyController::Up, pressed);
          break;
        case SDLK_DOWN:
          joypad.setButton(GameBoyController::Down, pressed);
          break;
        case SDLK_a:
          joypad.setButton(GameBoyController::A, pressed);
          break;
        case SDLK_b:
          joypad.setButton(GameBoyController::B, pressed);
          break;
        case SDLK_RSHIFT:
        case SDLK_LSHIFT:
          joypad.setButton(GameBoyController::Select, pressed);
          break;
        case SDLK_RETURN:
          joypad.setButton(GameBoyController::Start, pressed);
          break;
        default:
          break;
      }
    }
  }
}

void GameBoy::renderFrame() {
  static const uint32_t palette[4] = {
      0xFFFFFFFFu,  // white
      0xFFAAAAAAu,  // light grey
      0xFF555555u,  // dark grey
      0xFF000000u,  // black
  };

  const auto& framebuffer = bus.getFramebuffer();
  const uint8_t* shades = framebuffer.data();

  static uint32_t pixels[GB_WIDTH * GB_HEIGHT];
  for (int i = 0; i < GB_WIDTH * GB_HEIGHT; ++i) {
    pixels[i] = palette[shades[i] & 0x03];
  }

  SDL_UpdateTexture(texture, nullptr, pixels, GB_WIDTH * sizeof(uint32_t));
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

int GameBoy::run() {
  isRunning = true;
  openAudio();

  const bool hasAudio = audioDevice != 0;
  bool audioStarted = false;
  if (hasAudio) {
    SDL_ClearQueuedAudio(audioDevice);
    SDL_PauseAudioDevice(audioDevice, 1);
  }

  const bool debug = SDL_getenv("EMU_DEBUG") != nullptr;
  const double perfFreq = static_cast<double>(SDL_GetPerformanceFrequency());
  auto now = [&]() { return SDL_GetPerformanceCounter() / perfFreq; };

  const double baseCyclesPerSample = kCpuHz / sampleRate;
  double cyclesPerSample = baseCyclesPerSample;
  const double syncCycles = kChunkFrames * baseCyclesPerSample;
  const Uint32 frameBytes = 2 * sizeof(float);

  std::vector<float> chunk;
  chunk.reserve(kChunkFrames * 4);

  double phase = 0.0;
  double sumL = 0.0;
  double sumR = 0.0;
  double weight = 0.0;

  double wallStart = now();
  double totalCycles = 0.0;
  double sinceSync = 0.0;

  double dbgNext = wallStart + 2.0;
  double presentSum = 0.0;
  double presentMax = 0.0;
  int presentCount = 0;
  double qMin = 1e9;
  double qMax = 0.0;
  int underruns = 0;
  int dropped = 0;

  while (isRunning) {
    const uint32_t cycles = bus.step();

    float l = 0.0f;
    float r = 0.0f;
    bus.getStereoSample(l, r);
    sumL += static_cast<double>(l) * cycles;
    sumR += static_cast<double>(r) * cycles;
    weight += cycles;

    phase += cycles;
    if (phase >= cyclesPerSample) {
      phase -= cyclesPerSample;
      chunk.push_back(static_cast<float>(sumL / weight));
      chunk.push_back(static_cast<float>(sumR / weight));
      sumL = sumR = weight = 0.0;
    }

    if (bus.consumeFrameReady()) {
      const double t0 = debug ? now() : 0.0;
      renderFrame();
      if (debug) {
        const double dt = (now() - t0) * 1000.0;
        presentSum += dt;
        presentMax = std::max(presentMax, dt);
        ++presentCount;
      }
    }

    totalCycles += cycles;
    sinceSync += cycles;
    if (sinceSync < syncCycles) {
      continue;
    }
    sinceSync -= syncCycles;

    if (hasAudio) {
      const Uint32 queued = SDL_GetQueuedAudioSize(audioDevice);
      const double queuedFrames = static_cast<double>(queued) / frameBytes;
      if (queued == 0 && audioStarted) ++underruns;
      if (debug) {
        qMin = std::min(qMin, queuedFrames * 1000.0 / sampleRate);
        qMax = std::max(qMax, queuedFrames * 1000.0 / sampleRate);
      }

      if (queuedFrames > 2.0 * kTargetFrames) {
        ++dropped;
      } else if (!chunk.empty()) {
        SDL_QueueAudio(audioDevice, chunk.data(),
                       static_cast<Uint32>(chunk.size() * sizeof(float)));
      }
      chunk.clear();

      if (!audioStarted && queuedFrames >= kTargetFrames) {
        SDL_PauseAudioDevice(audioDevice, 0);
        audioStarted = true;
      }

      const double err = (queuedFrames - kTargetFrames) / kTargetFrames;
      const double adj =
          std::clamp(err * kMaxRateAdjust, -kMaxRateAdjust, kMaxRateAdjust);
      cyclesPerSample = baseCyclesPerSample * (1.0 + adj);
    } else {
      chunk.clear();
    }

    handleInput();

    const double target = wallStart + totalCycles / kCpuHz;
    double t = now();
    if (t > target + kMaxCatchUpSeconds) {
      wallStart += t - target - kMaxCatchUpSeconds;
    }
    while (isRunning && target - now() > 0.001) {
      SDL_Delay(1);
      handleInput();
    }

    if (debug && now() >= dbgNext) {
      const double wall = now() - wallStart;
      std::fprintf(
          stderr,
          "[gb] emu=%.2fs wall=%.2fs  queue=%.0f..%.0f ms  underruns=%d"
          " dropped=%d  present avg=%.2f max=%.2f ms  rate%+.2f%%\n",
          totalCycles / kCpuHz, wall, qMin == 1e9 ? 0.0 : qMin, qMax, underruns,
          dropped, presentCount ? presentSum / presentCount : 0.0, presentMax,
          (cyclesPerSample / baseCyclesPerSample - 1.0) * 100.0);
      dbgNext = now() + 2.0;
      presentSum = presentMax = 0.0;
      presentCount = 0;
      qMin = 1e9;
      qMax = 0.0;
    }
  }

  if (hasAudio) {
    SDL_PauseAudioDevice(audioDevice, 1);
    SDL_ClearQueuedAudio(audioDevice);
  }
  closeAudio();
  return 0;
}
