#include <cassert>
#include <cstdint>

#include "emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.hh"
#include "emulators/timer/game_boy_timer/GameBoyTimer.hh"

int main() {
  GameBoyInterruptController interrupts;
  GameBoyTimer timer(&interrupts);

  // TAC = 0b101: enable TIMA and select 262144 Hz (one increment per 16
  // CPU T-cycles). TIMA must not increment once per raw CPU cycle.
  timer.write(0xFF07, 0x05);
  timer.tick(15);
  assert(timer.read(0xFF05) == 0);
  timer.tick(1);
  assert(timer.read(0xFF05) == 1);
  timer.tick(15);
  assert(timer.read(0xFF05) == 1);
  timer.tick(1);
  assert(timer.read(0xFF05) == 2);

  // Overflow reloads TMA and requests the timer interrupt.
  timer.write(0xFF05, 0xFF);
  timer.write(0xFF06, 0xA7);
  interrupts.writeIF(0);
  timer.tick(16);
  assert(timer.read(0xFF05) == 0xA7);
  assert((interrupts.readIF() & GameBoyInterruptController::Timer) != 0);

  // DIV advances once per 256 CPU T-cycles and resets on write.
  timer.reset();
  timer.tick(255);
  assert(timer.read(0xFF04) == 0);
  timer.tick(1);
  assert(timer.read(0xFF04) == 1);
  timer.write(0xFF04, 0);
  assert(timer.read(0xFF04) == 0);

  return 0;
}
