#include "chip8.hh"

#include <SDL2/SDL.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

uint8_t CHIP8_FONTSET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

Chip8::Chip8() {
  for (int i = 0; i < 80; ++i) {
    this->memory[i] = CHIP8_FONTSET[i];
  }
  this->I = 0;
  this->pc = 0x200;  // program counter starts at 0x200
  this->draw_flag = 0;
  this->delay_timer = 0;
  this->sound_timer = 0;
  this->sp = 0;
  this->renderer = nullptr;
  this->quit = false;
  this->running = true;
  this->draw_flag = 1;
  this->returnValue = 0;
  this->highResolutionMode = false;
  std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
  std::fill(std::begin(this->V), std::end(this->V), 0);
  std::fill(std::begin(this->stack), std::end(this->stack), 0);
  std::fill(std::begin(this->key), std::end(this->key), 0);
  this->V[0xF] = 0;  // Reset VF register
}

void Chip8::setRenderer(SDL_Renderer* renderer) { this->renderer = renderer; }

void Chip8::audioCallback(void* userdata, Uint8* stream, int len) {
  auto* self = static_cast<Chip8*>(userdata);
  auto* samples = reinterpret_cast<int16_t*>(stream);
  const int count = len / sizeof(int16_t);

  const double sampleRate = 44100.0;
  const double tone = 440.0;
  const float volume = 3000.0f;
  const float target = self->beepOn.load() ? 1.0f : 0.0f;
  const float step =
      static_cast<float>(1.0 / (0.003 * sampleRate));  // rampe d'environ 3 ms

  for (int i = 0; i < count; ++i) {
    if (self->gain < target)
      self->gain = std::min<float>(self->gain + step, target);
    else if (self->gain > target)
      self->gain = std::max<float>(self->gain - step, target);

    float wave = self->phase < 0.5 ? 1.0f : -1.0f;
    samples[i] = static_cast<int16_t>(wave * volume * self->gain);

    self->phase += tone / sampleRate;
    if (self->phase >= 1.0) self->phase -= 1.0;
  }
}

void Chip8::initAudio() {
  if (this->audioDevice != 0) return;
  if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
    std::cerr << "Audio init failed: " << SDL_GetError() << '\n';
    return;
  }

  SDL_AudioSpec want{}, have{};
  want.freq = 44100;
  want.format = AUDIO_S16SYS;
  want.channels = 1;
  want.samples = 256;  // petit buffer = moins de latence
  want.callback = Chip8::audioCallback;
  want.userdata = this;

  this->audioDevice = SDL_OpenAudioDevice(nullptr, 0, &want, &have,
                                          SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (this->audioDevice == 0) {
    std::cerr << "Could not open audio device: " << SDL_GetError() << '\n';
    SDL_QuitSubSystem(SDL_INIT_AUDIO);  // undo SDL_InitSubSystem above
    return;
  }
  SDL_PauseAudioDevice(this->audioDevice, 0);  // tourne en permanence
}

void Chip8::setBeep(bool on) { this->beepOn.store(on); }

void Chip8::shutdownAudio() {
  if (this->audioDevice != 0) {
    SDL_PauseAudioDevice(this->audioDevice, 1);
    SDL_CloseAudioDevice(this->audioDevice);  // waits for the audio thread
    this->audioDevice = 0;
    if (SDL_WasInit(SDL_INIT_AUDIO)) SDL_QuitSubSystem(SDL_INIT_AUDIO);
  }
}

Chip8::~Chip8() { this->shutdownAudio(); }

void Chip8::updateTimers() {
  if (this->delay_timer > 0) {
    --this->delay_timer;
  }
  if (this->sound_timer > 0) {
    --this->sound_timer;
  }
  bool minHold = std::chrono::steady_clock::now() < this->beepUntil;
  this->setBeep(this->sound_timer > 0 || minHold);
}

void Chip8::loadProgram(const std::string& filename) {
  this->I = 0;
  this->pc = 0x200;  // program counter starts at 0x200
  this->draw_flag = 0;
  this->delay_timer = 0;
  this->sound_timer = 0;
  this->sp = 0;
  this->quit = false;
  this->running = true;
  this->highResolutionMode = false;
  this->draw_flag = 1;

  std::fill(std::begin(this->memory), std::end(this->memory), 0);
  std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
  std::fill(std::begin(this->V), std::end(this->V), 0);
  std::fill(std::begin(this->stack), std::end(this->stack), 0);

  for (int i = 0; i < 80; ++i) {
    this->memory[i] = CHIP8_FONTSET[i];
  }

  std::ifstream file(filename, std::ios::binary | std::ios::ate);

  if (!file) {
    this->quit = true;
    std::cerr << "Could not open ROM: " << filename << '\n';
    return;
  }

  // get the size of the file and read it into memory starting at 0x200
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (static_cast<std::size_t>(size) > sizeof(this->memory) - 0x200) {
    this->quit = true;
    std::cerr << "ROM too large\n";
    return;
  }

  file.read(reinterpret_cast<char*>(&this->memory[0x200]), size);

  std::cout << "Loaded ROM: " << size << " bytes\n";
  file.close();

  SDL_RenderSetLogicalSize(this->renderer, 64, 32);
}

void Chip8::drawGraphics() {
  SDL_SetRenderDrawColor(this->renderer, 18, 18, 18, 255);

  SDL_RenderClear(this->renderer);

  SDL_SetRenderDrawColor(this->renderer, 230, 230, 230, 255);

  int scale = this->highResolutionMode ? 2 : 1;
  for (int y = 0; y < 32 * scale; ++y) {
    for (int x = 0; x < 64 * scale; ++x) {
      if (this->gfx[y * 64 * scale + x]) {
        SDL_Rect pixel{x, y, 1, 1};
        SDL_RenderFillRect(this->renderer, &pixel);
      }
    }
  }

  SDL_RenderPresent(this->renderer);
}

void Chip8::handleInput() {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        this->quit = true;
        this->running = false;
        this->returnValue = 1;
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
            event.window.event == SDL_WINDOWEVENT_RESIZED) {
          // Force SDL à recalculer le viewport avec la taille physique actuelle
          int w = this->highResolutionMode ? 128 : 64;
          int h = this->highResolutionMode ? 64 : 32;
          SDL_RenderSetLogicalSize(this->renderer, w, h);
        }
        break;
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            this->quit = true;
            this->running = false;
            break;
          case SDLK_SPACE:
            this->running = !this->running;
            break;
          case SDLK_0:
            this->key[0x0] = 1;
            break;
          case SDLK_1:
            this->key[0x1] = 1;
            break;
          case SDLK_2:
            this->key[0x2] = 1;
            break;
          case SDLK_3:
            this->key[0x3] = 1;
            break;
          case SDLK_c:
            this->key[0xC] = 1;
            break;
          case SDLK_4:
            this->key[0x4] = 1;
            break;
          case SDLK_5:
            this->key[0x5] = 1;
            break;
          case SDLK_6:
            this->key[0x6] = 1;
            break;
          case SDLK_d:
            this->key[0xD] = 1;
            break;
          case SDLK_7:
            this->key[0x7] = 1;
            break;
          case SDLK_8:
            this->key[0x8] = 1;
            break;
          case SDLK_9:
            this->key[0x9] = 1;
            break;
          case SDLK_e:
            this->key[0xE] = 1;
            break;
          case SDLK_a:
            this->key[0xA] = 1;
            break;
          case SDLK_b:
            this->key[0xB] = 1;
            break;
          case SDLK_f:
            this->key[0xF] = 1;
            break;
          default:
            break;  // Ignore other keys
        }
        break;

      default:
        break;
    }
  }
}

void Chip8::executeOpcode(uint16_t opcode) {
  uint16_t nnn = opcode & 0x0FFF;
  uint8_t nn = opcode & 0x00FF;
  uint8_t n = opcode & 0x000F;
  uint8_t x = (opcode & 0x0F00) >> 8;
  uint8_t y = (opcode & 0x00F0) >> 4;

  int scale = this->highResolutionMode ? 2 : 1;

  // All CHIP-8 RAM accesses go through here: a ROM can never read or write
  // outside the memory array, whatever I holds.
  auto ram = [this](std::size_t addr) -> uint8_t& {
    return this->memory[addr % sizeof(this->memory)];
  };

  switch (opcode & 0xF000) {
    case 0x0000:
      if ((opcode & 0xFFF0) == 0x00C0) {
        int lines = n;
        for (int y = 32 * scale - 1; y >= lines; --y) {
          for (int x = 0; x < 64 * scale; ++x) {
            this->gfx[y * 64 * scale + x] =
                this->gfx[(y - lines) * 64 * scale + x];
          }
        }
        for (int y = 0; y < lines; ++y) {
          for (int x = 0; x < 64 * scale; ++x) {
            this->gfx[y * 64 * scale + x] = 0;
          }
        }
        this->draw_flag = 1;
        break;
      }
      switch (opcode) {
        case 0x00E0:  // 00E0: Clear the display
          for (int i = 0; i < 64 * 32 * scale * scale; ++i) {
            this->gfx[i] = 0;
          }
          this->draw_flag = 1;
          break;
        case 0x00EE:  // 00EE: Return from subroutine
          if (this->sp == 0) {
            std::cerr << "Stack underflow\n";
            this->quit = true;
            this->running = false;
            break;
          }
          --this->sp;
          this->pc = this->stack[this->sp];
          break;
        case 0x00FB:  // 00FB: Scroll display right by 4 pixels
          for (int y = 0; y < 32 * scale; ++y) {
            for (int x = 64 * scale - 1; x >= 4; --x) {
              this->gfx[y * 64 * scale + x] =
                  this->gfx[y * 64 * scale + (x - 4)];
            }
            for (int x = 0; x < 4; ++x) {
              this->gfx[y * 64 * scale + x] = 0;
            }
          }
          this->draw_flag = 1;
          break;
        case 0x00FC:  // 00FC: Scroll display left by 4 pixels
          for (int y = 0; y < 32 * scale; ++y) {
            for (int x = 0; x < 64 * scale - 4; ++x) {
              this->gfx[y * 64 * scale + x] =
                  this->gfx[y * 64 * scale + (x + 4)];
            }
            for (int x = 64 * scale - 4; x < 64 * scale; ++x) {
              this->gfx[y * 64 * scale + x] = 0;
            }
          }
          this->draw_flag = 1;
          break;
        case 0x00FD:  // 00FD: Exit the emulator
          this->quit = true;
          this->running = false;
          break;
        case 0x00FE:  // 00FE: Set the display to low resolution (64x32)
          SDL_RenderSetLogicalSize(this->renderer, 64, 32);
          std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
          this->highResolutionMode = false;
          break;
        case 0x00FF:  // 00FF: Set the display to high resolution (128x64)
          SDL_RenderSetLogicalSize(this->renderer, 128, 64);
          std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
          this->highResolutionMode = true;
          break;
        default:
          std::cerr << "Unknown opcode [0x0000]: " << std::hex << opcode
                    << std::endl;
      }
      break;
    case 0x1000:  // 1NNN: Jump to address NNN
      this->pc = nnn;
      break;
    case 0x2000:  // 2NNN: Call subroutine at NNN
      if (static_cast<std::size_t>(this->sp) >= std::size(this->stack)) {
        std::cerr << "Stack overflow\n";
        this->quit = true;
        this->running = false;
        break;
      }
      this->stack[this->sp] = this->pc;
      ++this->sp;
      this->pc = nnn;
      break;
    case 0x3000:  // 3XNN: Skip next instruction if VX equals NN
      if (this->V[x] == nn) {
        this->pc += 2;
      }
      break;
    case 0x4000:  // 4XNN: Skip next instruction if VX doesn't equal NN
      if (this->V[x] != nn) {
        this->pc += 2;
      }
      break;
    case 0x5000:  // 5XY0: Skip next instruction if VX equals VY
      if (this->V[x] == this->V[y]) {
        this->pc += 2;
      }
      break;
    case 0x6000:  // 6XNN: Set VX to NN
      this->V[x] = nn;
      break;
    case 0x7000:  // 7XNN: Add NN to VX (carry flag is not changed)
      this->V[x] += nn;
      break;
    case 0x8000:
      if (n == 0x0) {  // 8XY0: Set VX to the value of VY
        this->V[x] = this->V[y];
      } else if (n == 0x1) {  // 8XY1: Set VX to VX OR VY
        this->V[x] |= this->V[y];
      } else if (n == 0x2) {  // 8XY2: Set VX to VX AND VY
        this->V[x] &= this->V[y];
      } else if (n == 0x3) {  // 8XY3: Set VX to VX XOR VY
        this->V[x] ^= this->V[y];
      } else if (n == 0x4) {  // 8XY4: Add VY to VX. VF is set to 1 when there's
                              // a carry, and to 0 when there isn't
        uint16_t sum = this->V[x] + this->V[y];
        this->V[0xF] = sum > 255 ? 1 : 0;
        this->V[x] = sum & 0xFF;
      } else if (n == 0x5) {  // 8XY5: Subtract VY from VX. VF is set to 0 when
                              // there's a borrow, and 1 when there isn't
        this->V[0xF] = this->V[x] > this->V[y] ? 1 : 0;
        this->V[x] -= this->V[y];
      } else if (n == 0x6) {  // 8XY6: Store the least significant bit of VX in
                              // VF and then shift VX to the right by 1
        this->V[0xF] = this->V[x] & 0x1;
        this->V[x] >>= 1;
      } else if (n == 0x7) {  // 8XY7: Set VX to VY minus VX. VF is set to 0
                              // when there's a borrow, and 1 when there isn't
        this->V[0xF] = this->V[y] > this->V[x] ? 1 : 0;
        this->V[x] = this->V[y] - this->V[x];
      } else if (n == 0xE) {  // 8XYE: Store the most significant bit of VX in
                              // VF and then shift VX to the left by 1
        this->V[0xF] = (this->V[x] & 0x80) >> 7;
        this->V[x] <<= 1;
      } else {
        std::cerr << "Unknown opcode [0x8000]: " << std::hex << opcode
                  << std::endl;
      }
      break;
    case 0x9000:  // 9XY0: Skip next instruction if VX doesn't equal VY
      if (this->V[x] != this->V[y]) {
        this->pc += 2;
      }
      break;
    case 0xA000:  // ANNN: Set I to the address NNN
      this->I = nnn;
      break;
    case 0xB000:  // BNNN: Jump to the address NNN
      this->pc = nnn + this->V[0];
      break;
    case 0xC000:  // CXNN: Set VX to a random number AND NN
      this->V[x] = (rand() % 256) & nn;
      break;
    case 0xD000:  // DXYN: Draw a sprite at position (VX, VY)
    {
      const int width = 64 * scale;
      const int height = 32 * scale;
      const int xPos = this->V[x] % width;
      const int yPos = this->V[y] % height;
      this->V[0xF] = 0;

      // High-res DXY0 draws a 16x16 sprite (2 bytes per row).
      const bool big = (n == 0 && this->highResolutionMode);
      const int rows = big ? 16 : n;
      const int cols = big ? 16 : 8;

      for (int row = 0; row < rows; ++row) {
        const int py = yPos + row;
        if (py >= height) break;  // clip at the bottom edge

        const int base = this->I + (big ? row * 2 : row);
        const unsigned bits =
            big ? static_cast<unsigned>((ram(base) << 8) | ram(base + 1))
                : static_cast<unsigned>(ram(base) << 8);

        for (int col = 0; col < cols; ++col) {
          const int px = xPos + col;
          if (px >= width) break;  // clip at the right edge
          if ((bits & (0x8000u >> col)) == 0) continue;

          const std::size_t idx = static_cast<std::size_t>(py * width + px);
          if (idx >= std::size(this->gfx)) continue;  // never write past gfx
          if (this->gfx[idx] == 1) this->V[0xF] = 1;
          this->gfx[idx] ^= 1;
        }
      }
      this->draw_flag = 1;
      break;
    }
    case 0xE000:
      if (nn == 0x9E) {  // EX9E: Skip next instruction if the key stored in VX
                         // is pressed
        if (this->key[this->V[x] & 0x0F] != 0) {
          this->pc += 2;
          this->key[this->V[x] & 0x0F] =
              0;  // Clear the key state after processing
        }
      } else if (nn == 0xA1) {  // EXA1: Skip next instruction if the key stored
                                // in VX isn't pressed
        if (this->key[this->V[x] & 0x0F] == 0) {
          this->pc += 2;
        }
      } else {
        std::cerr << "Unknown opcode [0xE000]: " << std::hex << opcode
                  << std::endl;
      }
      break;
    case 0xF000:
      switch (nn) {
        case 0x07:  // FX07: Set VX to the value of the delay timer
          this->V[x] = this->delay_timer;
          break;
        case 0x0A: {  // FX0A: Wait for a key press
          bool keyPressed = false;
          for (int i = 0; i < 16; ++i) {
            if (this->key[i] != 0) {
              this->V[x] = i;
              keyPressed = true;
              this->key[i] = 0;  // Clear the key state after processing
              break;
            }
          }
          if (!keyPressed) {
            this->pc -= 2;  // repeat this instruction until a key is pressed
          }
          break;
        }
        case 0x15:  // FX15: Set the delay timer to VX
          this->delay_timer = this->V[x];
          break;
        case 0x18:  // FX18: Set the sound timer to VX
          this->sound_timer = this->V[x];
          if (this->sound_timer > 0) {
            std::cerr << "FX18 beep, timer=" << int(this->sound_timer) << '\n';
            this->beepUntil =
                std::chrono::steady_clock::now() +
                std::chrono::milliseconds(100);  // minimum beep length
            this->setBeep(true);
          }
          break;
        case 0x1E:  // FX1E: Add VX to I
          this->I += this->V[x];
          break;
        case 0x29:  // FX29: Set I to the location of the sprite for the
                    // character in VX
          this->I = this->V[x] * 5;
          break;
        case 0x30:  // FX30: Set I to the location of the 10-byte font sprite
                    // for the character in VX
          this->I = this->V[x] * 10;
          break;
        case 0x33:  // FX33: Store the binary-coded decimal representation of VX
          ram(this->I) = this->V[x] / 100;
          ram(this->I + 1) = (this->V[x] / 10) % 10;
          ram(this->I + 2) = this->V[x] % 10;
          break;
        case 0x55:  // FX55: Store V0 to VX in memory starting
          for (int i = 0; i <= x; ++i) {
            ram(this->I + i) = this->V[i];
          }
          break;
        case 0x65:  // FX65: Read V0 to VX from memory starting
          for (int i = 0; i <= x; ++i) {
            this->V[i] = ram(this->I + i);
          }
          break;
        case 0x75:    // FX75: Store V0 to VX in RPL user flags
          if (x < 8)  // Only allow storing up to V7 in RPL user flags
          {
            for (int i = 0; i <= x; ++i) {
              this->rpl[i] = this->V[i];
            }
          }
          break;
        case 0x85:    // FX85: Read V0 to VX from RPL user flags
          if (x < 8)  // Only allow storing up to V7 in RPL user flags
          {
            for (int i = 0; i <= x; ++i) {
              this->V[i] = this->rpl[i];
            }
          }
          break;
        default:
          std::cerr << "Unknown opcode [0xF000]: " << std::hex << opcode
                    << std::endl;
      }
      break;
    default:
      std::cerr << "Unknown opcode: " << std::hex << opcode << std::endl;
  }
}

void Chip8::clearKeyStates() {
  for (int i = 0; i < 16; ++i) {
    this->key[i] = 0;
  }
}

void Chip8::cycle() {
  const std::size_t addr = this->pc % sizeof(this->memory);
  uint16_t opcode = (this->memory[addr] << 8) |
                    this->memory[(addr + 1) % sizeof(this->memory)];
  this->pc = static_cast<uint16_t>(addr + 2);
  this->executeOpcode(opcode);
}

int Chip8::run() {
  using clock = std::chrono::steady_clock;
  constexpr auto timerPeriod = std::chrono::microseconds(16667);  // ~60 Hz

  this->initAudio();

  auto lastTimerUpdate = clock::now();

  while (!this->quit) {
    this->handleInput();

    if (!this->running) {
      // En pause : on ne consomme pas de CPU et on fige la référence des timers
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      lastTimerUpdate = clock::now();
      continue;
    }

    this->cycle();

    auto now = clock::now();
    if (now - lastTimerUpdate >= timerPeriod) {
      this->updateTimers();
      lastTimerUpdate += timerPeriod;
    }

    if (this->draw_flag) {
      this->drawGraphics();
      this->draw_flag = 0;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // The audio thread uses `this`: stop it before the object can be destroyed.
  this->shutdownAudio();
  return this->returnValue;
}
