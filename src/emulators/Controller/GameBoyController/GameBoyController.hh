#pragma once

#include <cstdint>

#include "InterruptController.hh"

// 0xFF00. Buttons are active-low in hardware; this class exposes a clean
// active-high API (setButton(Right, true) == pressed) and does the
// inversion internally.
class GameBoyController {
 public:
  enum Button : uint8_t {
    Right = 0,
    Left = 1,
    Up = 2,
    Down = 3,
    A = 4,
    B = 5,
    Select = 6,
    Start = 7,
  };

  explicit GameBoyController(InterruptController* interrupts)
      : interrupts(interrupts) {}

  void reset() {
    buttons = 0x00;
    selectBits = 0x30;
  }

  void setButton(Button button, bool pressed) {
    bool wasUnset = !(buttons & (1 << button));
    if (pressed)
      buttons |= (1 << button);
    else
      buttons &= ~(1 << button);

    // A newly-pressed button while its group is selected fires the
    // joypad interrupt (used to wake the CPU from HALT).
    if (pressed && wasUnset) interrupts->request(InterruptController::Joypad);
  }

  uint8_t read() const {
    uint8_t result = 0xC0 | selectBits;
    bool selectDirections = !(selectBits & 0x10);
    bool selectActions = !(selectBits & 0x20);
    uint8_t lowNibble = 0x0F;
    if (selectDirections) lowNibble &= ~(buttons & 0x0F);
    if (selectActions) lowNibble &= ~((buttons >> 4) & 0x0F);
    return result | lowNibble;
  }

  void write(uint8_t value) { selectBits = value & 0x30; }

 private:
  InterruptController* interrupts;
  uint8_t buttons =
      0x00;  // bit set = pressed, bits 0-3 dpad, 4-7 A/B/Sel/Start
  uint8_t selectBits = 0x30;  // bits 4-5 of 0xFF00, active-low select lines
};
