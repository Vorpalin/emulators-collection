#pragma once

#include <cstdint>

class InterruptController {
 public:
  enum Flag : uint8_t {
    VBlank = 1 << 0,
    LCDStat = 1 << 1,
    Timer = 1 << 2,
    Serial = 1 << 3,
    Joypad = 1 << 4,
  };

  void reset() {
    ifReg = 0x00;
    ieReg = 0x00;
  }

  void request(Flag flag) { ifReg |= flag; }

  uint8_t readIF() const { return 0xE0 | ifReg; }  // top 3 bits float high
  void writeIF(uint8_t value) { ifReg = value & 0x1F; }

  uint8_t readIE() const { return ieReg; }
  void writeIE(uint8_t value) { ieReg = value; }

  // Interrupts that are both requested and enabled; the CPU services the
  // lowest-numbered bit set here and clears it via writeIF.
  uint8_t pending() const { return ifReg & ieReg & 0x1F; }

 private:
  uint8_t ifReg = 0x00;
  uint8_t ieReg = 0x00;
};
