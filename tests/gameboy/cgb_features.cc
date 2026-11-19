// Game Boy Color feature tests on tiny hand-assembled ROMs (no download).
// Each ROM runs a short program that stores what it observes in WRAM
// (0xC000...), which is then checked through the bus.
//
// Covered: KEY1 speed switch (+ 2x CPU throughput), SVBK WRAM banking,
// general-purpose HDMA, HBlank HDMA, VRAM bank 1, palette auto-increment, and
// the fact that none of these registers exist on a DMG-only cartridge.
#include <cstdint>
#include <cstdio>
#include <vector>

#include "emulators/console/game_boy/GameBoy.hh"

using Bytes = std::vector<uint8_t>;

static Bytes operator+(Bytes a, const Bytes& b) {
  a.insert(a.end(), b.begin(), b.end());
  return a;
}
static Bytes LdA(uint8_t v) { return {0x3E, v}; }
static Bytes LdhWr(uint8_t a) { return {0xE0, a}; }
static Bytes LdhRd(uint8_t a) { return {0xF0, a}; }
static Bytes St(uint16_t a) {
  return {0xEA, static_cast<uint8_t>(a & 0xFF), static_cast<uint8_t>(a >> 8)};
}
static Bytes LdFrom(uint16_t a) {
  return {0xFA, static_cast<uint8_t>(a & 0xFF), static_cast<uint8_t>(a >> 8)};
}
// Busy loop: LD B,0xFF ; DEC B ; JR NZ,-3, repeated `n` times (~9 lines each).
static Bytes Spin(int n) {
  Bytes b;
  for (int i = 0; i < n; ++i) b = b + Bytes{0x06, 0xFF, 0x05, 0x20, 0xFD};
  return b;
}

static Bytes BuildRom(const Bytes& code, uint8_t cgbFlag) {
  Bytes rom(0x8000, 0);
  const Bytes entry = {0x00, 0xC3, 0x50, 0x01};  // NOP ; JP 0x0150
  for (std::size_t i = 0; i < entry.size(); ++i) rom[0x100 + i] = entry[i];
  rom[0x143] = cgbFlag;
  uint8_t sum = 0;
  for (int i = 0x134; i < 0x14D; ++i)
    sum = static_cast<uint8_t>(sum - rom[i] - 1);
  rom[0x14D] = sum;
  for (std::size_t i = 0; i < code.size(); ++i) rom[0x150 + i] = code[i];
  return rom;
}

static int failures = 0;
static void Expect(const char* what, int got, int want) {
  if (got != want) {
    std::printf("FAIL %-34s got %02x want %02x\n", what, got, want);
    ++failures;
  } else {
    std::printf("ok   %-34s %02x\n", what, got);
  }
}

static void Run(GameBoy& gb, const Bytes& rom, int frames) {
  gb.loadProgram(rom.data(), rom.size());
  for (int i = 0; i < frames; ++i) gb.stepFrame();
}

int main() {
  // --- Main feature program -------------------------------------------------
  Bytes code;
  // 1. Arm KEY1, STOP -> double speed; read KEY1 back.
  code = code + LdA(1) + LdhWr(0x4D) + Bytes{0x10, 0x00} + LdhRd(0x4D) +
         St(0xC000);
  // 2. SVBK: banks 2 and 3 are distinct, SVBK=0 selects bank 1.
  code = code + LdA(2) + LdhWr(0x70) + LdA(0xAA) + St(0xD000);
  code = code + LdA(3) + LdhWr(0x70) + LdA(0xBB) + St(0xD000);
  code = code + LdA(2) + LdhWr(0x70) + LdFrom(0xD000) + St(0xC001);
  code = code + LdA(3) + LdhWr(0x70) + LdFrom(0xD000) + St(0xC002);
  code = code + LdA(0) + LdhWr(0x70) + LdFrom(0xD000) + St(0xC003);
  // 3. General-purpose HDMA WRAM 0xC100 -> VRAM 0x8000, and VRAM banking.
  code = code + LdA(0x5A) + St(0xC100);
  code = code + LdA(0xC1) + LdhWr(0x51) + LdA(0x00) + LdhWr(0x52);
  code = code + LdA(0x00) + LdhWr(0x53) + LdA(0x00) + LdhWr(0x54);
  code = code + LdA(0x00) + LdhWr(0x55);
  code = code + LdFrom(0x8000) + St(0xC004);
  code = code + LdhRd(0x55) + St(0xC008);
  code = code + LdA(1) + LdhWr(0x4F) + LdA(0x77) + St(0x8000);
  code = code + LdA(0) + LdhWr(0x4F) + LdFrom(0x8000) + St(0xC005);
  code = code + LdA(1) + LdhWr(0x4F) + LdFrom(0x8000) + St(0xC006);
  // 4. Palette index auto-increment (a read must not increment).
  code = code + LdA(0x80) + LdhWr(0x68) + LdA(0x1F) + LdhWr(0x69) + LdA(0x00) +
         LdhWr(0x69);
  code = code + LdA(0x80) + LdhWr(0x68) + LdhRd(0x69) + St(0xC007);
  code = code + LdhRd(0x68) + St(0xC009);
  code = code + Bytes{0x18, 0xFE};  // JR -2

  GameBoy cgb;
  Run(cgb, BuildRom(code, 0x80), 10);
  auto& bus = cgb.getBus();
  std::printf("== CGB cartridge\n");
  Expect("cgb mode detected", bus.cgbMode(), 1);
  Expect("double speed active", bus.doubleSpeed(), 1);
  Expect("KEY1 after switch", bus.read(0xC000), 0xFE);
  Expect("SVBK bank 2", bus.read(0xC001), 0xAA);
  Expect("SVBK bank 3", bus.read(0xC002), 0xBB);
  Expect("SVBK 0 -> bank 1 (untouched)", bus.read(0xC003), 0x00);
  Expect("general HDMA copied", bus.read(0xC004), 0x5A);
  Expect("HDMA5 idle after copy", bus.read(0xC008), 0xFF);
  Expect("VRAM bank 0 intact", bus.read(0xC005), 0x5A);
  Expect("VRAM bank 1 independent", bus.read(0xC006), 0x77);
  Expect("palette read no increment", bus.read(0xC007), 0x1F);
  Expect("BCPS keeps auto-inc bit", bus.read(0xC009), 0xC0);

  // --- DMG-only cartridge: CGB registers must not exist ----------------------
  GameBoy dmg;
  Run(dmg, BuildRom(code, 0x00), 10);
  std::printf("== DMG cartridge\n");
  Expect("cgb mode off", dmg.getBus().cgbMode(), 0);
  Expect("KEY1 reads 0xFF", dmg.getBus().read(0xC000), 0xFF);
  Expect("HDMA5 reads 0xFF", dmg.getBus().read(0xC008), 0xFF);
  Expect("no double speed", dmg.getBus().doubleSpeed(), 0);

  // --- HBlank HDMA: 2 blocks, one per HBlank ---------------------------------
  Bytes h;
  h = h + Bytes{0x21, 0x00, 0xC1, 0x3E, 0x10};        // LD HL,C100 ; LD A,10
  h = h + Bytes{0x22, 0x3C, 0xFE, 0x30, 0x20, 0xFA};  // fill 0x10..0x2F
  h = h + LdA(0xC1) + LdhWr(0x51) + LdA(0x00) + LdhWr(0x52);
  h = h + LdA(0x10) + LdhWr(0x53) + LdA(0x00) + LdhWr(0x54);  // -> 0x9000
  h = h + LdA(0x81) + LdhWr(0x55);   // HBlank DMA, 2 blocks
  h = h + LdhRd(0x55) + St(0xC000);  // just started
  h = h + Spin(60);                  // > a few frames
  h = h + LdhRd(0x55) + St(0xC001);
  h = h + LdFrom(0x9000) + St(0xC002);
  h = h + LdFrom(0x901F) + St(0xC003);
  h = h + Bytes{0x18, 0xFE};
  GameBoy hb;
  Run(hb, BuildRom(h, 0x80), 10);
  std::printf("== HBlank HDMA\n");
  Expect("HDMA5 right after start", hb.getBus().read(0xC000) & 0x80, 0x00);
  Expect("HDMA5 when finished", hb.getBus().read(0xC001), 0xFF);
  Expect("first byte copied", hb.getBus().read(0xC002), 0x10);
  Expect("last byte copied", hb.getBus().read(0xC003), 0x2F);

  // A block started by the PPU's HBlank callback must stall the CPU during the
  // same bus step. Otherwise the next CPU instruction runs before the queued
  // eight-M-cycle DMA stall is consumed.
  for (uint16_t i = 0; i < 16; ++i)
    hb.getBus().write(static_cast<uint16_t>(0xC100 + i),
                      static_cast<uint8_t>(0xA0 + i));
  auto dmaState = hb.getBus().getState();
  dmaState.ppu.regs.setMode(PixelTransfer);
  dmaState.ppu.regs.ly = 0;
  dmaState.ppu.dotCounter = 170;  // next 12 dots enter HBlank
  dmaState.hdmaSource = 0xC100;
  dmaState.hdmaDest = 0x0100;
  dmaState.hdmaRemaining = 1;
  dmaState.hdmaActive = true;
  hb.getBus().setState(dmaState);
  const uint32_t hblankStepCycles = hb.getBus().step();
  std::printf("== HBlank DMA stall timing\n");
  Expect("callback stall included in same step", hblankStepCycles, 44);
  Expect("callback DMA copied first byte", hb.getBus().read(0x9000), 0xA0);

  // --- OAM DMA: incremental transfer and CPU bus restriction ---------------
  Bytes oam;
  oam = oam + LdA(0) + LdhWr(0x40);  // LCD off; OAM is readable.
  oam = oam + Bytes{0x21, 0x00, 0xC1, 0x06, 0xA0, 0xAF};
  oam = oam + Bytes{0x22, 0x3C, 0x05, 0x20, 0xFB};  // Fill C100-C19F.
  oam = oam + LdA(0xC1) + LdhWr(0x46);
  oam = oam + Bytes{0x18, 0xFE};  // CPU remains in ROM while DMA runs.
  GameBoy oamGb;
  const Bytes oamRom = BuildRom(oam, 0x00);
  oamGb.loadProgram(oamRom.data(), oamRom.size());
  auto& oamBus = oamGb.getBus();
  oamBus.write(0xFF40, 0);
  oamBus.write(0xFE00, 0xEE);
  for (int i = 0; i < 2000 && !oamBus.getState().dmaActive; ++i) oamBus.step();
  std::printf("== OAM DMA\n");
  Expect("OAM DMA started", oamBus.getState().dmaActive, 1);
  Expect("DMA blocks WRAM reads", oamBus.read(0xC100), 0xFF);
  Expect("DMA permits HRAM reads", oamBus.read(0xFF80), 0x00);
  Expect("OAM DMA has not copied instantly", oamBus.getState().ppu.oam[0],
         0xEE);
  for (int i = 0; i < 100 && oamBus.getState().dmaActive; ++i) oamBus.step();
  Expect("OAM DMA completed", oamBus.getState().dmaActive, 0);
  Expect("OAM DMA first byte", oamBus.read(0xFE00), 0x00);
  Expect("OAM DMA last byte", oamBus.read(0xFE9F), 0x9F);

  // --- Double speed doubles CPU throughput, not video time -------------------
  const Bytes loop = {0x21, 0x00, 0x00,
                      0x23, 0x18, 0xFD};  // LD HL,0 ; INC HL ; JR
  const Bytes sw = LdA(1) + LdhWr(0x4D) + Bytes{0x10, 0x00} + loop;
  GameBoy slow, fast;
  Run(slow, BuildRom(loop, 0x80), 4);
  Run(fast, BuildRom(sw, 0x80), 4);
  auto hl = [](GameBoy& g) {
    const auto s = g.getBus().getState();
    return (s.cpu.H << 8) | s.cpu.L;
  };
  const int a = hl(slow), b = hl(fast);  // 4 frames: neither counter wraps
  std::printf("== Speed\nnormal=%d double=%d ratio=%.2f\n", a, b,
              a ? static_cast<double>(b) / a : 0.0);
  if (a == 0 || b < a * 18 / 10 || b > a * 22 / 10) {
    std::printf("FAIL double speed should run ~2x the instructions\n");
    ++failures;
  }

  std::printf("%s (%d failure%s)\n", failures ? "FAILED" : "PASSED", failures,
              failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
