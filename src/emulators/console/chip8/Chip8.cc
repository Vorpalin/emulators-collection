#include "emulators/console/chip8/Chip8.hh"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>

namespace {

constexpr uint8_t CHIP8_FONTSET[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0,  // 0
    0x20, 0x60, 0x20, 0x20, 0x70,  // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0,  // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0,  // 3
    0x90, 0x90, 0xF0, 0x10, 0x10,  // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0,  // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0,  // 6
    0xF0, 0x10, 0x20, 0x40, 0x40,  // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0,  // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0,  // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90,  // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0,  // B
    0xF0, 0x80, 0x80, 0x80, 0xF0,  // C
    0xE0, 0x90, 0x90, 0x90, 0xE0,  // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0,  // E
    0xF0, 0x80, 0xF0, 0x80, 0x80   // F
};

// Display colors (same as the former SDL renderer).
constexpr uint8_t kBg[3] = {18, 18, 18};
constexpr uint8_t kFg[3] = {230, 230, 230};

}  // namespace

Chip8::Chip8() {
  this->resetState();
  this->audio_.reserve(4096);
}

void Chip8::resetState() {
  std::fill(std::begin(this->memory), std::end(this->memory), 0);
  std::copy(std::begin(CHIP8_FONTSET), std::end(CHIP8_FONTSET),
            std::begin(this->memory));

  std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
  std::fill(std::begin(this->V), std::end(this->V), 0);
  std::fill(std::begin(this->stack), std::end(this->stack), 0);
  std::fill(std::begin(this->key), std::end(this->key), 0);
  std::fill(std::begin(this->rpl), std::end(this->rpl), 0);

  this->I = 0;
  this->pc = 0x200;  // program counter starts at 0x200
  this->sp = 0;
  this->delay_timer = 0;
  this->sound_timer = 0;
  this->draw_flag = 1;
  this->lastPressedKey = -1;
  this->halted = false;
  this->highResolutionMode = false;

  this->beepHoldFrames_ = 0;
  this->beepOn_ = false;
  this->gain_ = 0.0f;
  this->phase_ = 0.0;
  this->sampleAcc_ = 0.0;
  this->audio_.clear();

  this->renderFramebuffer();
  this->draw_flag = 0;
}

bool Chip8::loadProgram(const uint8_t* data, std::size_t size) {
  this->resetState();
  this->rom_.clear();

  if (data == nullptr || size == 0) {
    std::cerr << "Empty ROM\n";
    this->halted = true;
    return false;
  }
  if (size > sizeof(this->memory) - 0x200) {
    std::cerr << "ROM too large\n";
    this->halted = true;
    return false;
  }

  this->rom_.assign(data, data + size);
  std::copy(data, data + size, &this->memory[0x200]);
  return true;
}

void Chip8::reset() {
  if (this->rom_.empty()) {
    this->resetState();
    return;
  }
  // Copy: loadProgram() clears rom_ before re-assigning it.
  const std::vector<uint8_t> rom = this->rom_;
  this->loadProgram(rom.data(), rom.size());
}

void Chip8::setAudioSampleRate(int hz) {
  if (hz > 0) this->sampleRate_ = static_cast<double>(hz);
}

void Chip8::setKey(int keyIndex, bool pressed) {
  if (keyIndex < 0 || keyIndex > 15) return;
  this->key[keyIndex] = pressed ? 1 : 0;
  if (pressed) this->lastPressedKey = keyIndex;
}

void Chip8::updateTimers() {
  if (this->delay_timer > 0) {
    --this->delay_timer;
  }
  if (this->sound_timer > 0) {
    --this->sound_timer;
  }
  if (this->beepHoldFrames_ > 0) {
    --this->beepHoldFrames_;
  }
  this->beepOn_ = this->sound_timer > 0 || this->beepHoldFrames_ > 0;
}

void Chip8::renderFramebuffer() {
  const int pixels = this->width() * this->height();
  for (int i = 0; i < pixels; ++i) {
    const uint8_t* c = this->gfx[i] ? kFg : kBg;
    uint8_t* out = &this->framebuffer_[static_cast<std::size_t>(i) * 4];
    out[0] = c[0];
    out[1] = c[1];
    out[2] = c[2];
    out[3] = 255;
  }
}

void Chip8::generateAudio() {
  this->audio_.clear();

  // One video frame lasts 1/60 s; carry the fractional part between frames.
  this->sampleAcc_ += this->sampleRate_ / 60.0;
  const int frames = static_cast<int>(this->sampleAcc_);
  this->sampleAcc_ -= frames;

  const float target = this->beepOn_ ? 1.0f : 0.0f;
  const float step =
      static_cast<float>(1.0 / (kRampSeconds * this->sampleRate_));

  for (int i = 0; i < frames; ++i) {
    if (this->gain_ < target)
      this->gain_ = std::min(this->gain_ + step, target);
    else if (this->gain_ > target)
      this->gain_ = std::max(this->gain_ - step, target);

    const float wave = this->phase_ < 0.5 ? 1.0f : -1.0f;
    const float sample = wave * kVolume * this->gain_;
    this->audio_.push_back(sample);  // left
    this->audio_.push_back(sample);  // right

    this->phase_ += kToneHz / this->sampleRate_;
    if (this->phase_ >= 1.0) this->phase_ -= 1.0;
  }
}

void Chip8::stepFrame() {
  if (!this->halted) {
    for (int i = 0; i < kCyclesPerFrame && !this->halted; ++i) {
      this->cycle();
    }
    this->updateTimers();
  } else {
    this->beepOn_ = false;  // fade out the beep once the program ended
  }

  if (this->draw_flag) {
    this->renderFramebuffer();
    this->draw_flag = 0;
  }

  this->generateAudio();

  // A key tap is only visible to FX0A during the frame that follows it.
  this->lastPressedKey = -1;
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
      if ((opcode & 0xFFF0) == 0x00C0) {  // 00CN: Scroll display down N lines
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
            this->halted = true;
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
        case 0x00FD:  // 00FD: Exit the interpreter
          this->halted = true;
          break;
        case 0x00FE:  // 00FE: Set the display to low resolution (64x32)
          std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
          this->highResolutionMode = false;
          this->draw_flag = 1;
          break;
        case 0x00FF:  // 00FF: Set the display to high resolution (128x64)
          std::fill(std::begin(this->gfx), std::end(this->gfx), 0);
          this->highResolutionMode = true;
          this->draw_flag = 1;
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
        this->halted = true;
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
      // Key states now come from setKey() with real press/release events,
      // so (unlike the SDL version) the keys are not cleared after a test.
      if (nn == 0x9E) {  // EX9E: Skip next instruction if the key stored in VX
                         // is pressed
        if (this->key[this->V[x] & 0x0F] != 0) {
          this->pc += 2;
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
          if (this->lastPressedKey >= 0) {
            this->V[x] = static_cast<uint8_t>(this->lastPressedKey);
            this->lastPressedKey = -1;
          } else {
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
            this->beepHoldFrames_ = kBeepHoldFrames;  // minimum beep length
            this->beepOn_ = true;
          }
          break;
        case 0x1E:  // FX1E: Add VX to I
          this->I += this->V[x];
          break;
        case 0x29:  // FX29: Set I to the location of the sprite for the
                    // character in VX
          this->I = (V[x] & 0xF) * 5;
          break;
        case 0x30:  // FX30: Set I to the location of the 10-byte font sprite
                    // for the character in VX
          this->I = 0x50 + (this->V[x] % 10) * 10;
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

void Chip8::cycle() {
  const std::size_t addr = this->pc % sizeof(this->memory);
  uint16_t opcode = (this->memory[addr] << 8) |
                    this->memory[(addr + 1) % sizeof(this->memory)];
  this->pc = static_cast<uint16_t>(addr + 2);
  this->executeOpcode(opcode);
}
