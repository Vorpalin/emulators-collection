#include <cassert>
#include <cstdint>

#include "emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.hh"
#include "emulators/timer/game_boy_timer/GameBoyTimer.hh"

int main() {
  GameBoyInterruptController interrupts;
  GameBoyTimer timer(&interrupts);

  // TAC = 0b101 selects DIV bit 3: TIMA increments on its falling edge,
  // once every 16 CPU T-cycles.
  timer.write(0xFF07, 0x05);
  timer.tick(15);
  assert(timer.read(0xFF05) == 0);
  timer.tick(1);
  assert(timer.read(0xFF05) == 1);
  timer.tick(15);
  assert(timer.read(0xFF05) == 1);
  timer.tick(1);
  assert(timer.read(0xFF05) == 2);

  // All TAC frequencies derive from DIV falling edges.
  timer.reset();
  timer.write(0xFF07, 0x06);  // DIV bit 5 -> 64 T-cycles
  timer.tick(63);
  assert(timer.read(0xFF05) == 0);
  timer.tick(1);
  assert(timer.read(0xFF05) == 1);
  timer.reset();
  timer.write(0xFF07, 0x07);  // DIV bit 7 -> 256 T-cycles
  timer.tick(255);
  assert(timer.read(0xFF05) == 0);
  timer.tick(1);
  assert(timer.read(0xFF05) == 1);
  timer.reset();
  timer.write(0xFF07, 0x04);  // DIV bit 9 -> 1024 T-cycles
  timer.tick(1023);
  assert(timer.read(0xFF05) == 0);
  timer.tick(1);
  assert(timer.read(0xFF05) == 1);

  // DIV exposes the upper byte of the 16-bit internal divider.
  timer.reset();
  timer.tick(255);
  assert(timer.read(0xFF04) == 0);
  timer.tick(1);
  assert(timer.read(0xFF04) == 1);
  timer.write(0xFF04, 0);
  assert(timer.read(0xFF04) == 0);

  // Resetting DIV while the selected timer input is high creates a falling
  // edge and therefore increments TIMA.
  timer.reset();
  timer.write(0xFF07, 0x05);
  timer.tick(8);  // DIV bit 3 becomes high.
  assert(timer.read(0xFF05) == 0);
  timer.write(0xFF04, 0);
  assert(timer.read(0xFF05) == 1);

  // Changing TAC can also create a falling edge.
  timer.reset();
  timer.tick(8);  // DIV bit 3 is high.
  timer.write(0xFF07, 0x05);
  assert(timer.read(0xFF05) == 1);

  // Overflow is delayed by four T-cycles; TIMA can be written to cancel it.
  timer.reset();
  timer.write(0xFF06, 0xA7);
  timer.write(0xFF05, 0xFF);
  timer.write(0xFF07, 0x05);
  timer.tick(16);
  assert(timer.read(0xFF05) == 0x00);
  assert((interrupts.readIF() & GameBoyInterruptController::Timer) == 0);
  timer.tick(3);
  assert(timer.read(0xFF05) == 0x00);
  assert((interrupts.readIF() & GameBoyInterruptController::Timer) == 0);
  timer.tick(1);
  assert(timer.read(0xFF05) == 0xA7);
  assert((interrupts.readIF() & GameBoyInterruptController::Timer) != 0);

  // A TIMA write during the pending reload window cancels the reload/IRQ.
  timer.reset();
  interrupts.writeIF(0);
  timer.write(0xFF06, 0xA7);
  timer.write(0xFF05, 0xFF);
  timer.write(0xFF07, 0x05);
  timer.tick(16);
  timer.write(0xFF05, 0x42);
  timer.tick(4);
  assert(timer.read(0xFF05) == 0x42);
  assert((interrupts.readIF() & GameBoyInterruptController::Timer) == 0);

  // Save-state round trip preserves divider phase and pending reload.
  timer.reset();
  timer.write(0xFF05, 0xFF);
  timer.write(0xFF07, 0x05);
  timer.tick(16);
  auto state = timer.getState();
  GameBoyTimer restored(&interrupts);
  restored.setState(state);
  restored.tick(4);
  assert(restored.read(0xFF05) == 0);
  return 0;
}
