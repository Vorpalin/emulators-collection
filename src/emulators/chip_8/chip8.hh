#pragma once

#include <cstdint>
#include <string>

#include "emulator.hh"

struct SDL_Renderer;
struct SDL_Window;

class Chip8 : public Emulator
{
public:
  Chip8();

  int run() override;

  void loadProgram(const std::string& filename) override;
  void setRenderer(SDL_Renderer* renderer) override;

private:
  void handleInput();

  void clearKeyStates();

  // CHIP-8 timers run at 60 Hz, independently of the CPU cycle rate.
  void updateTimers();

  void drawGraphics();

  // execute the opcode
  void executeOpcode(uint16_t opcode);

  // Fetch, decode, and execute one instruction.
  void cycle();

  // the CHIP-8 has 4K memory (4096 bytes)
  //
  // memory map:
  //      0x000-0x1FF - Chip 8 interpreter (contains font set in emu)
  //      0x050-0x0A0 - Used for the built in 4x5 pixel font set (0-F)
  //      0x200-0xFFF - Program ROM and work RAM
  //
  uint8_t memory[4096];

  // the CHIP-8 has 16 8-bit registers (V0 to VF)
  uint8_t V[16];

  // the index register
  uint16_t I;

  // the program counter
  uint16_t pc;

  // the CHIP-8 has a 64x32 pixel monochrome display
  uint8_t gfx[128 * 64]; // 128x64 for high-resolution mode

  // determines if the screen needs to be redrawn
  uint8_t draw_flag;

  // the CHIP-8 has two timers which count down at 60Hz
  uint8_t delay_timer;
  uint8_t sound_timer;

  // the CHIP-8 has a stack which is used to store return addresses when subroutines are called
  uint16_t stack[16];

  // the stack pointer which points to the top of the stack
  uint16_t sp;

  // the CHIP-8 has a hexadecimal keypad with 16 keys (0x0-0xF)
  uint8_t key[16];
  uint8_t rpl[16];

  SDL_Renderer* renderer;

  bool running;
  bool quit;

  int returnValue;

  bool highResolutionMode; // Flag to indicate if high-resolution mode is active
};