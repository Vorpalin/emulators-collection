// MasterSystem / GameGear console checks through the generic Console
// interface: ROM loading, picture size and RGBA output, audio production,
// key mapping, battery RAM and save states.
//
// The test ROMs run a small Z80 program that mirrors the joypad into the
// backdrop colour once per frame, which proves the whole path
// setKey() -> I/O port -> Z80 -> VDP palette -> RGBA framebuffer.

#include <cstdint>
#include <cstdio>
#include <vector>

#include "emulators/console/game_gear/GameGear.hh"
#include "emulators/console/master_system/MasterSystem.hh"

namespace {

int failures = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
      failures++;                                                      \
    }                                                                  \
  } while (0)

struct Asm {
  std::vector<uint8_t> b;
  void raw(std::initializer_list<uint8_t> v) { b.insert(b.end(), v); }
  void out(uint8_t port, uint8_t value) { raw({0x3E, value, 0xD3, port}); }
  void vdpReg(int r, uint8_t v) {
    out(0xBF, v);
    out(0xBF, static_cast<uint8_t>(0x80 | r));
  }
};

void put(std::vector<uint8_t>& rom, size_t at, const std::vector<uint8_t>& v) {
  for (uint8_t x : v) rom[at++] = x;
}

// Master System: every frame, joypad 1 (active low) is inverted, combined
// with a "paused" flag kept in RAM and used directly as the 6-bit palette
// entry 0, so each button lights one colour bit. Pause (NMI) sets the flag
// to 0x30, which paints the backdrop blue.
std::vector<uint8_t> smsRom() {
  std::vector<uint8_t> rom(0x8000, 0);
  put(rom, 0x0000, {0xF3, 0xED, 0x56, 0xC3, 0x00, 0x01});
  put(rom, 0x0038,
      {0xF5, 0xE5,        // PUSH AF ; PUSH HL
       0xDB, 0xBF,        // IN A,(BF)   acknowledge the VDP
       0xDB, 0xDC,        // IN A,(DC)   joypad
       0x2F,              // CPL
       0xE6, 0x3F,        // AND 3F
       0x21, 0x00, 0xC0,  // LD HL,C000  paused flag
       0xB6,              // OR (HL)
       0xF5,              // PUSH AF
       0xAF,              // XOR A
       0xD3, 0xBF,        // OUT (BF),A
       0x3E, 0xC0,        // LD A,C0     CRAM address 0
       0xD3, 0xBF,        // OUT (BF),A
       0xF1,              // POP AF
       0xD3, 0xBE,        // OUT (BE),A  colour 0
       0xE1, 0xF1,        // POP HL ; POP AF
       0xFB,              // EI
       0xC9});            // RET
  put(rom, 0x0066,
      {0x21, 0x00, 0xC0,  // LD HL,C000
       0x36, 0x30,        // LD (HL),30
       0xED, 0x45});      // RETN
  Asm a;
  a.vdpReg(0, 0x04);
  a.vdpReg(1, 0x60);  // Display + frame interrupt.
  a.raw({0xFB, 0x76, 0x18, 0xFD});  // EI ; loop: HALT ; JR loop
  put(rom, 0x0100, a.b);
  return rom;
}

// Game Gear: Start (port 0, bit 7, active low) lights the red bit of the
// 12-bit palette entry 0.
std::vector<uint8_t> ggRom() {
  std::vector<uint8_t> rom(0x8000, 0);
  put(rom, 0x0000, {0xF3, 0xED, 0x56, 0xC3, 0x00, 0x01});
  put(rom, 0x0038,
      {0xF5, 0xDB, 0xBF,  // PUSH AF ; IN A,(BF)
       0xDB, 0x00,        // IN A,(00)   Start button
       0x2F, 0xE6, 0x80,  // CPL ; AND 80
       0x07,              // RLCA        bit 7 -> bit 0 (red = 1)
       0xF5, 0xAF, 0xD3, 0xBF, 0x3E, 0xC0, 0xD3, 0xBF, 0xF1,
       0xD3, 0xBE,        // low byte of colour 0
       0xAF, 0xD3, 0xBE,  // high byte
       0xF1, 0xFB, 0xC9});
  Asm a;
  a.vdpReg(0, 0x04);
  a.vdpReg(1, 0x60);
  a.raw({0xFB, 0x76, 0x18, 0xFD});
  put(rom, 0x0100, a.b);
  return rom;
}

bool load(Console& c, const std::vector<uint8_t>& rom) {
  return c.loadProgram(rom.data(), rom.size());
}

void frames(Console& c, int n) {
  for (int i = 0; i < n; i++) c.stepFrame();
}

struct Pixel {
  int r, g, b, a;
};

Pixel pixel(const Console& c, int x, int y) {
  const uint8_t* p = c.framebuffer() + (static_cast<size_t>(y) * static_cast<size_t>(c.width()) + static_cast<size_t>(x)) * 4;
  return {p[0], p[1], p[2], p[3]};
}

void testLoadingAndPicture() {
  MasterSystem sms;
  CHECK(!sms.loadProgram(nullptr, 0));
  CHECK(!sms.error().empty());
  CHECK(!sms.hasBattery());  // Nothing loaded yet.
  sms.stepFrame();           // Harmless without a ROM.
  CHECK(sms.audioFrameCount() == 0);
  CHECK(!sms.loadState("{}"));
  CHECK(!sms.isHalted());

  CHECK(load(sms, smsRom()));
  CHECK(sms.width() == 256 && sms.height() == 192);
  frames(sms, 3);
  const Pixel p = pixel(sms, 100, 100);
  CHECK(p.a == 255);
  // Colour 0 is still black (no button pressed), fully opaque.
  CHECK(p.r == 0 && p.g == 0 && p.b == 0);

  GameGear gg;
  CHECK(load(gg, ggRom()));
  CHECK(gg.width() == 160 && gg.height() == 144);
  CHECK(GameGear::kWidth == 160 && GameGear::kHeight == 144);
  frames(gg, 3);
  CHECK(pixel(gg, 159, 143).a == 255);
}

void testAudioProduction() {
  MasterSystem sms;
  CHECK(load(sms, smsRom()));
  sms.stepFrame();  // The first frame is shorter: it starts at line 0.
  CHECK(sms.audioFrameCount() > 500 && sms.audioFrameCount() < 700);
  sms.stepFrame();
  const size_t n48 = sms.audioFrameCount();
  CHECK(n48 > 780 && n48 < 820);  // 48000 / 59.92 Hz.

  sms.setAudioSampleRate(44100);
  sms.stepFrame();
  const size_t n44 = sms.audioFrameCount();
  CHECK(n44 > 715 && n44 < 755);  // 44100 / 59.92 Hz.
  CHECK(sms.audioSamples() != nullptr);

  sms.setPAL(true);
  sms.setAudioSampleRate(48000);
  sms.stepFrame();
  sms.stepFrame();
  const size_t pal = sms.audioFrameCount();
  CHECK(pal > 940 && pal < 990);  // 48000 / 49.7 Hz.
}

void testKeyMapping() {
  struct Case {
    int key;
    int r, g, b;
  };
  // Joypad bit n lights palette bit n: R = bits 0-1, G = bits 2-3, B = 4-5.
  // A 2-bit channel value of 1 is 10/31 -> 82, of 2 is 21/31 -> 173.
  const Case cases[] = {
      {MasterSystem::kUp, 82, 0, 0},     {MasterSystem::kDown, 173, 0, 0},
      {MasterSystem::kLeft, 0, 82, 0},   {MasterSystem::kRight, 0, 173, 0},
      {MasterSystem::kA, 0, 0, 82},      {MasterSystem::kB, 0, 0, 173},
  };
  for (const Case& c : cases) {
    MasterSystem sms;
    CHECK(load(sms, smsRom()));
    sms.setKey(c.key, true);
    frames(sms, 3);
    const Pixel p = pixel(sms, 10, 10);
    CHECK(p.r == c.r && p.g == c.g && p.b == c.b);
    sms.setKey(c.key, false);
    frames(sms, 3);
    const Pixel q = pixel(sms, 10, 10);
    CHECK(q.r == 0 && q.g == 0 && q.b == 0);
  }

  // Select and unknown keys are ignored.
  MasterSystem sms;
  CHECK(load(sms, smsRom()));
  sms.setKey(MasterSystem::kSelect, true);
  sms.setKey(99, true);
  sms.setKey(-1, true);
  frames(sms, 3);
  const Pixel p = pixel(sms, 10, 10);
  CHECK(p.r == 0 && p.g == 0 && p.b == 0);

  // Start is Pause on the Master System: the NMI paints the backdrop blue
  // (31 -> 255 on the blue channel for the 2-bit colour 3).
  MasterSystem pause;
  CHECK(load(pause, smsRom()));
  frames(pause, 3);
  pause.setKey(MasterSystem::kStart, true);
  pause.stepFrame();
  const Pixel b = pixel(pause, 10, 10);
  CHECK(b.r == 0 && b.g == 0 && b.b == 255);

  // On the Game Gear, Start is a real button.
  GameGear gg;
  CHECK(load(gg, ggRom()));
  frames(gg, 3);
  CHECK(pixel(gg, 10, 10).r == 0);
  gg.setKey(GameGear::kStart, true);
  frames(gg, 3);
  const Pixel s = pixel(gg, 10, 10);
  CHECK(s.r == 16 && s.g == 0 && s.b == 0);  // 4-bit red 1 -> 5-bit 2 -> 16.
  gg.setKey(GameGear::kStart, false);
  frames(gg, 3);
  CHECK(pixel(gg, 10, 10).r == 0);
}

void testBatteryRam() {
  MasterSystem sms;
  CHECK(load(sms, smsRom()));
  CHECK(sms.hasBattery());
  std::vector<uint8_t> ram = sms.saveRam();
  CHECK(ram.size() == 0x8000);
  ram[0] = 0x5A;
  CHECK(sms.loadRam(ram.data(), ram.size()));
  CHECK(!sms.consumeRamDirty());
  CHECK(sms.saveRam()[0] == 0x5A);
  CHECK(!sms.loadRam(ram.data(), 100));

  MasterSystem empty;
  CHECK(!empty.loadRam(ram.data(), ram.size()));
  CHECK(!empty.consumeRamDirty());
}

void testStateAndReset() {
  const std::vector<uint8_t> rom = smsRom();
  MasterSystem a;
  CHECK(load(a, rom));
  a.setKey(MasterSystem::kRight, true);
  frames(a, 4);
  const std::string state = a.saveState();
  CHECK(!state.empty());

  MasterSystem b;
  CHECK(load(b, rom));
  CHECK(b.loadState(state));
  frames(a, 3);
  frames(b, 3);
  const size_t bytes = static_cast<size_t>(a.width() * a.height()) * 4;
  CHECK(std::vector<uint8_t>(a.framebuffer(), a.framebuffer() + bytes) ==
        std::vector<uint8_t>(b.framebuffer(), b.framebuffer() + bytes));
  CHECK(a.audioFrameCount() == b.audioFrameCount());
  CHECK(std::vector<float>(a.audioSamples(), a.audioSamples() + a.audioFrameCount() * 2) ==
        std::vector<float>(b.audioSamples(), b.audioSamples() + b.audioFrameCount() * 2));
  CHECK(a.saveState() == b.saveState());

  CHECK(!b.loadState("not json"));
  CHECK(!b.loadState("{\"cpu\": 3}"));

  // Reset keeps the ROM but clears the program's progress: the held key is
  // forgotten, so the backdrop goes back to black after a few frames.
  a.reset();
  frames(a, 3);
  const Pixel p = pixel(a, 10, 10);
  CHECK(p.r == 0 && p.g == 0 && p.b == 0);
}

}  // namespace

int main() {
  testLoadingAndPicture();
  testAudioProduction();
  testKeyMapping();
  testBatteryRam();
  testStateAndReset();
  if (failures == 0) std::printf("master_system_console: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
