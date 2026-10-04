#pragma once

#include <cstdint>

struct GameBoyInterruptControllerState {
  uint8_t ifReg;
  uint8_t ieReg;
};

class GameBoyInterruptController {
 public:
  enum Flag : uint8_t {
    VBlank = 1 << 0,
    LCDStat = 1 << 1,
    Timer = 1 << 2,
    Serial = 1 << 3,
    Joypad = 1 << 4,
  };

  void reset();

  void request(Flag flag);

  uint8_t readIF() const;  // top 3 bits float high
  void writeIF(uint8_t value);

  uint8_t readIE() const;
  void writeIE(uint8_t value);

  // Interrupts that are both requested and enabled; the CPU services the
  // lowest-numbered bit set here and clears it via writeIF.
  uint8_t pending() const;

  void setState(const GameBoyInterruptControllerState& state);
  GameBoyInterruptControllerState getState() const;

 private:
  uint8_t ifReg = 0x00;
  uint8_t ieReg = 0x00;
};
