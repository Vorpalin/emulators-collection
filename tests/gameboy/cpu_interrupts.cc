// Synthetic CPU interrupt regressions. No external ROMs are required.
#include <array>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "emulators/console/game_boy/GameBoy.hh"

namespace {
int failures = 0;

void expect(const char* name, int actual, int expected) {
  if (actual != expected) {
    std::printf("FAIL %-42s got %02X, expected %02X\n", name, actual, expected);
    ++failures;
  } else {
    std::printf("ok   %-42s %02X\n", name, actual);
  }
}

void loadProgram(GameBoy& gb, const std::array<uint8_t, 3>& program) {
  std::vector<uint8_t> rom(0x8000, 0);
  rom[0x100] = program[0];
  rom[0x101] = program[1];
  rom[0x102] = program[2];
  rom[0x147] = 0x00;  // ROM only
  rom[0x148] = 0x00;  // 32 KiB
  rom[0x149] = 0x00;  // no RAM
  gb.loadProgram(rom.data(), rom.size());
}

void testHaltBugSuppressesOneOpcodeFetchIncrement() {
  GameBoy gb;
  loadProgram(gb, {0x76, 0x3E, 0x42});  // HALT; LD A,42
  auto& bus = gb.getBus();
  bus.interrupts.writeIE(GameBoyInterruptController::VBlank);
  bus.interrupts.request(GameBoyInterruptController::VBlank);

  bus.step();  // HALT sees IME=0 and an enabled interrupt already pending.
  auto state = bus.getState().cpu;
  expect("HALT bug does not enter halted state", state.halted, 0);
  expect("HALT bug leaves PC after HALT", state.PC, 0x101);
  expect("HALT bug is armed", state.haltBug, 1);

  bus.step();  // Fetch 0x3E without incrementing PC; immediate is also 0x3E.
  state = bus.getState().cpu;
  expect("HALT bug is consumed once", state.haltBug, 0);
  expect("HALT bug duplicates opcode as immediate", state.A, 0x3E);
  expect("HALT bug advances PC by one byte", state.PC, 0x102);
}

void testHaltWakesWhenEnabledInterruptBecomesPending() {
  GameBoy gb;
  loadProgram(gb, {0x76, 0x00, 0x00});  // HALT; NOP; NOP
  auto& bus = gb.getBus();
  bus.interrupts.writeIE(GameBoyInterruptController::Joypad);

  bus.step();
  auto state = bus.getState().cpu;
  expect("HALT without pending interrupt enters halt", state.halted, 1);
  expect("HALT advances past opcode", state.PC, 0x101);

  bus.step();  // Remains halted.
  expect("HALT remains asleep without interrupt", bus.getState().cpu.PC, 0x101);

  bus.interrupts.request(GameBoyInterruptController::Joypad);
  bus.step();  // IME=0: wakes, but does not dispatch the interrupt.
  state = bus.getState().cpu;
  expect("Pending interrupt wakes HALT with IME=0", state.halted, 0);
  expect("Woken CPU executes next opcode", state.PC, 0x102);
}
}  // namespace

int main() {
  testHaltBugSuppressesOneOpcodeFetchIncrement();
  testHaltWakesWhenEnabledInterruptBecomesPending();
  return failures == 0 ? 0 : 1;
}
