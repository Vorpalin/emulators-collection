#pragma once

#include <cstdint>

#include "InterruptController.hh"

class GameBoyTimer {
 public:
  explicit GameBoyTimer(InterruptController* interrupts)
      : interrupts(interrupts) {
    divider = 0;
    timerCounter = 0;
    timerModulo = 0;
    timerControl = 0;
    timerCycles = 0;
    dividerCounter = 0;
  }
  GameBoyTimer();

  void reset();
  void tick(uint8_t cycles);
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);

 private:
  uint32_t timerThreshold() const;
  uint16_t divider;                 // Divider register (0xFF04)
  uint8_t timerCounter;             // Timer counter (0xFF05)
  uint8_t timerModulo;              // Timer modulo (0xFF06)
  uint8_t timerControl;             // Timer control (0xFF07)
  uint16_t timerCycles;             // Internal counter for timer ticks
  uint32_t dividerCounter;          // Internal counter for divider ticks
  InterruptController* interrupts;  // Handles timer interrupts
};
