#pragma once

#include <cstdint>

#include "InterruptController.hh"

class GameBoyTimer {
 public:
  GameBoyTimer();

  void reset();
  void tick(uint8_t cycles);
  uint8_t read(uint16_t address);
  void write(uint16_t address, uint8_t value);

 private:
  uint32_t timerThreshold() const;
  uint16_t divider;         // Divider register (0xFF04)
  uint8_t timerCounter;     // Timer counter (0xFF05)
  uint8_t timerModulo;      // Timer modulo (0xFF06)
  uint8_t timerControl;     // Timer control (0xFF07)
  uint16_t timerCycles;     // Internal counter for timer ticks
  uint32_t dividerCounter;  // Internal counter for divider ticks
  InterruptController interruptController;  // Handles timer interrupts
};
