// Checks the battery-RAM API of Console on tiny synthetic ROMs (no external
// files): a program writes 0x42 to cartridge RAM; we verify that the write is
// reported as a change exactly once, that the RAM can be exported and
// restored into a fresh console, and that carts without a battery refuse it.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "emulators/console/game_boy/GameBoy.hh"
#include "emulators/console/nes/Nes.hh"

static int failures = 0;
#define CHECK(cond)                                                        \
  do {                                                                     \
    if (!(cond)) {                                                         \
      std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
      ++failures;                                                          \
    }                                                                      \
  } while (0)

// 32 KiB ROM. Program at 0x150: enable RAM, store 0x42 at 0xA000 forever.
static std::vector<uint8_t> MakeGameBoyRom(uint8_t cartType) {
  std::vector<uint8_t> rom(0x8000, 0);
  rom[0x100] = 0x00;  // nop
  rom[0x101] = 0xC3;  // jp 0x0150
  rom[0x102] = 0x50;
  rom[0x103] = 0x01;
  const uint8_t code[] = {
      0x3E, 0x0A,        // ld a, 0x0A
      0xEA, 0x00, 0x00,  // ld (0x0000), a   ; enable cartridge RAM
      0x3E, 0x42,        // loop: ld a, 0x42
      0xEA, 0x00, 0xA0,  // ld (0xA000), a
      0x18, 0xF9,        // jr loop
  };
  for (std::size_t i = 0; i < sizeof(code); ++i) rom[0x150 + i] = code[i];
  rom[0x147] = cartType;
  rom[0x148] = 0x00;  // 32 KiB
  rom[0x149] = 0x02;  // 8 KiB RAM
  uint8_t sum = 0;
  for (int i = 0x134; i <= 0x14C; ++i)
    sum = static_cast<uint8_t>(sum - rom[i] - 1);
  rom[0x14D] = sum;
  return rom;
}

// iNES, mapper 0, 16 KiB PRG + 8 KiB CHR. Program: lda #$42 / sta $6000 / jmp.
static std::vector<uint8_t> MakeNesRom(bool battery) {
  std::vector<uint8_t> rom(16 + 0x4000 + 0x2000, 0);
  const uint8_t header[16] = {
      'N', 'E', 'S', 0x1A, 1, 1, static_cast<uint8_t>(battery ? 0x02 : 0x00)};
  for (int i = 0; i < 16; ++i) rom[i] = header[i];
  const uint8_t code[] = {0xA9, 0x42, 0x8D, 0x00, 0x60, 0x4C, 0x05, 0x80};
  for (std::size_t i = 0; i < sizeof(code); ++i) rom[16 + i] = code[i];
  rom[16 + 0x3FFC] = 0x00;  // reset vector -> $8000
  rom[16 + 0x3FFD] = 0x80;
  return rom;
}

static void RunFrames(Console& c, int n) {
  for (int i = 0; i < n; ++i) c.stepFrame();
}

static void TestBatteryRoundTrip(const char* name, Console& a, Console& b,
                                 const std::vector<uint8_t>& rom,
                                 std::size_t expectedSize) {
  std::printf("%s\n", name);
  CHECK(a.loadProgram(rom.data(), rom.size()));
  CHECK(a.hasBattery());
  CHECK(!a.consumeRamDirty());  // nothing written yet

  RunFrames(a, 3);
  CHECK(a.consumeRamDirty());   // the game wrote to RAM
  CHECK(!a.consumeRamDirty());  // flag is cleared by reading it
  RunFrames(a, 3);
  CHECK(!a.consumeRamDirty());  // rewriting the same value is not a change

  const std::vector<uint8_t> ram = a.saveRam();
  CHECK(ram.size() == expectedSize);
  CHECK(!ram.empty() && ram[0] == 0x42);

  CHECK(b.loadProgram(rom.data(), rom.size()));
  CHECK(b.saveRam()[0] == 0x00);                  // fresh cart is empty
  CHECK(!b.loadRam(ram.data(), ram.size() - 1));  // wrong size is refused
  CHECK(b.loadRam(ram.data(), ram.size()));
  CHECK(b.saveRam() == ram);    // restored exactly
  CHECK(!b.consumeRamDirty());  // restoring is not a save
}

int main() {
  {
    GameBoy a, b;
    TestBatteryRoundTrip("Game Boy MBC1+RAM+BATTERY", a, b,
                         MakeGameBoyRom(0x03), 0x2000);
  }
  {
    GameBoy g;  // MBC1+RAM, no battery: RAM exists but must not be persisted
    const auto rom = MakeGameBoyRom(0x02);
    CHECK(g.loadProgram(rom.data(), rom.size()));
    CHECK(!g.hasBattery());
    RunFrames(g, 3);
    const std::vector<uint8_t> dummy(0x2000, 1);
    CHECK(!g.loadRam(dummy.data(), dummy.size()));
  }
  {
    Nes a, b;
    TestBatteryRoundTrip("NES with battery", a, b, MakeNesRom(true), 0x2000);
  }
  {
    Nes n;
    const auto rom = MakeNesRom(false);
    CHECK(n.loadProgram(rom.data(), rom.size()));
    CHECK(!n.hasBattery());
    RunFrames(n, 3);
    CHECK(!n.consumeRamDirty());
    CHECK(n.saveRam().empty());
  }

  if (failures == 0) std::printf("all battery checks passed\n");
  return failures == 0 ? 0 : 1;
}
