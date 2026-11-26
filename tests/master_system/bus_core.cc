// Master System bus checks: memory map and mapper banking, cartridge RAM,
// Codemasters detection, I/O ports and joypads, Game Gear ports, and the
// whole machine running a small hand-assembled Z80 program (VDP set-up,
// frame interrupts, NMI from the Pause button, timing and save states).

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "emulators/bus/master_system_bus/MasterSystemBus.hh"

namespace {

int failures = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
      failures++;                                                      \
    }                                                                  \
  } while (0)

constexpr uint16_t kRed = 31;

// ROM whose page k is filled with `base + k`.
std::vector<uint8_t> pagedRom(size_t pages, uint8_t base) {
  std::vector<uint8_t> rom(pages * SegaCartridge::kPageSize);
  for (size_t p = 0; p < pages; p++)
    for (size_t i = 0; i < SegaCartridge::kPageSize; i++)
      rom[p * SegaCartridge::kPageSize + i] = static_cast<uint8_t>(base + p);
  return rom;
}

bool load(MasterSystemBus &bus, const std::vector<uint8_t> &rom) {
  return bus.loadROM(rom.data(), rom.size());
}

// Tiny Z80 assembler helpers.
struct Asm {
  std::vector<uint8_t> b;
  void raw(std::initializer_list<uint8_t> v) { b.insert(b.end(), v); }
  void out(uint8_t port, uint8_t value) { raw({0x3E, value, 0xD3, port}); }
  void vdpReg(int r, uint8_t v) {
    out(0xBF, v);
    out(0xBF, static_cast<uint8_t>(0x80 | r));
  }
  void vdpAddr(uint16_t a, int code) {
    out(0xBF, static_cast<uint8_t>(a & 0xFF));
    out(0xBF, static_cast<uint8_t>((code << 6) | (a >> 8)));
  }
};

// 32 KiB ROM: reset vector, IM 1 interrupt handler, NMI handler and a main
// program that draws one red tile and then halts, counting frame interrupts
// in RAM[0] and NMIs in RAM[1].
std::vector<uint8_t> demoRom() {
  std::vector<uint8_t> rom(0x8000, 0);
  auto put = [&](size_t at, std::initializer_list<uint8_t> v) {
    for (uint8_t x : v) rom[at++] = x;
  };
  put(0x0000, {0xF3, 0xED, 0x56, 0xC3, 0x00, 0x01});  // DI ; IM 1 ; JP 0100
  // IRQ handler: acknowledge the VDP, increment the counter at 0xC000.
  put(0x0038, {0xF5, 0xE5, 0xDB, 0xBF, 0x21, 0x00, 0xC0, 0x34, 0xE1, 0xF1,
               0xFB, 0xC9});
  // NMI handler: increment the counter at 0xC001.
  put(0x0066, {0x21, 0x01, 0xC0, 0x34, 0xED, 0x45});

  Asm a;
  a.vdpReg(0, 0x04);  // Mode 4.
  a.vdpReg(1, 0x40);  // Display on.
  a.vdpReg(2, 0xFF);  // Name table at 0x3800.
  a.vdpReg(5, 0xFF);  // Sprite table at 0x3F00.
  a.vdpAddr(0, 3);    // CRAM: colour 0 black, colour 1 red.
  a.out(0xBE, 0x00);
  a.out(0xBE, 0x03);
  a.vdpAddr(0x20, 1);  // Tile 1: eight rows of colour 1.
  a.raw({0x06, 0x08});  // LD B,8
  a.raw({0x3E, 0xFF, 0xD3, 0xBE, 0xAF, 0xD3, 0xBE, 0xD3, 0xBE, 0xD3, 0xBE});
  a.raw({0x10, 0xF3});  // DJNZ -13
  a.vdpAddr(0x3800, 1);  // Name table entry (0,0) = tile 1.
  a.out(0xBE, 0x01);
  a.out(0xBE, 0x00);
  a.vdpReg(1, 0x60);  // Display on + frame interrupt.
  a.raw({0xFB});      // EI
  a.raw({0x76, 0x18, 0xFD});  // loop: HALT ; JR loop
  put(0x0100, {});
  for (size_t i = 0; i < a.b.size(); i++) rom[0x100 + i] = a.b[i];
  return rom;
}

void runFrames(MasterSystemBus &bus, int frames) {
  for (int f = 0; f < frames; f++) {
    for (int guard = 0; guard < 100000 && !bus.consumeFrameReady(); guard++)
      bus.step();
  }
}

void testMapperBanking() {
  MasterSystemBus bus;
  CHECK(load(bus, pagedRom(8, 0x10)));
  CHECK(bus.cartridge().mapper() == SegaCartridge::Mapper::Sega);
  CHECK(bus.cartridge().pageCount() == 8);
  CHECK(bus.read(0x0000) == 0x10);  // Default: pages 0, 1, 2.
  CHECK(bus.read(0x4000) == 0x11);
  CHECK(bus.read(0x8000) == 0x12);

  bus.write(0xFFFF, 5);
  CHECK(bus.read(0x8000) == 0x15);
  bus.write(0xFFFE, 3);
  CHECK(bus.read(0x4000) == 0x13);
  bus.write(0xFFFD, 4);
  CHECK(bus.read(0x0500) == 0x14);
  CHECK(bus.read(0x0100) == 0x10);  // The first 1 KiB stays on page 0.
  bus.write(0xFFFF, 9);             // Wraps around the 8 pages.
  CHECK(bus.read(0x8000) == 0x11);

  // RAM, its mirror, and the mapper registers living in that mirror.
  bus.write(0xC123, 0xAB);
  CHECK(bus.read(0xE123) == 0xAB);
  bus.write(0xE456, 0xCD);
  CHECK(bus.read(0xC456) == 0xCD);
  CHECK(bus.read(0xFFFF) == 9);
  CHECK(bus.read(0xFFFD) == 4);

  // Reset restores the default banks and clears RAM.
  bus.reset();
  CHECK(bus.read(0x8000) == 0x12 && bus.read(0xC123) == 0);

  // Memory control: the cartridge slot can be disabled.
  bus.writePort(0x3E, 0xEB);  // Bit 6 set: cartridge off.
  CHECK(bus.read(0x0000) == 0xFF);
  bus.writePort(0x3E, 0xAB);
  CHECK(bus.read(0x0000) == 0x10);
}

void testCopierHeaderAndErrors() {
  MasterSystemBus bus;
  CHECK(!bus.loadROM(nullptr, 0));
  CHECK(!bus.error().empty());
  CHECK(!bus.cartridge().loaded());

  std::vector<uint8_t> rom(512, 0xEE);  // Copier header.
  std::vector<uint8_t> body = pagedRom(2, 0x40);
  rom.insert(rom.end(), body.begin(), body.end());
  CHECK(bus.loadROM(rom.data(), rom.size()));
  CHECK(bus.cartridge().pageCount() == 2);
  CHECK(bus.read(0x0000) == 0x40 && bus.read(0x4000) == 0x41);

  // A ROM smaller than one page is padded.
  std::vector<uint8_t> tiny(100, 0x77);
  CHECK(bus.loadROM(tiny.data(), tiny.size()));
  CHECK(bus.read(0x0010) == 0x77 && bus.read(0x0200) == 0xFF);
}

void testCartridgeRam() {
  MasterSystemBus bus;
  CHECK(load(bus, pagedRom(4, 0x10)));
  CHECK(bus.read(0x8000) == 0x12);  // ROM until RAM is mapped.

  bus.write(0xFFFC, 0x08);  // Map RAM bank 0 into slot 2.
  bus.write(0x8000, 0x77);
  CHECK(bus.read(0x8000) == 0x77);
  bus.write(0xFFFC, 0x0C);  // RAM bank 1.
  CHECK(bus.read(0x8000) == 0);
  bus.write(0x8000, 0x88);
  bus.write(0xFFFC, 0x08);
  CHECK(bus.read(0x8000) == 0x77);
  bus.write(0xFFFC, 0x00);  // Unmapped: ROM again, writes are ignored.
  bus.write(0x8000, 0x99);
  CHECK(bus.read(0x8000) == 0x12);

  CHECK(bus.cartridge().hasBattery());
  CHECK(bus.cartridge().consumeRamDirty());
  CHECK(!bus.cartridge().consumeRamDirty());
  std::vector<uint8_t> ram = bus.cartridge().batteryRam();
  CHECK(ram.size() == 0x8000 && ram[0] == 0x77 && ram[0x4000] == 0x88);

  // Restore into a fresh machine (32 KiB, then a 16 KiB image).
  MasterSystemBus other;
  CHECK(load(other, pagedRom(4, 0x10)));
  CHECK(other.cartridge().loadBatteryRam(ram.data(), ram.size()));
  other.write(0xFFFC, 0x08);
  CHECK(other.read(0x8000) == 0x77);
  CHECK(!other.cartridge().consumeRamDirty());
  CHECK(other.cartridge().loadBatteryRam(ram.data(), 0x4000));
  other.write(0xFFFC, 0x0C);
  CHECK(other.read(0x8000) == 0);
  CHECK(!other.cartridge().loadBatteryRam(ram.data(), 123));
}

void testCodemasters() {
  std::vector<uint8_t> rom = pagedRom(4, 0x20);
  // Valid header at 0x7FE0: checksum = sum of the 16-bit words outside it.
  for (size_t i = 0x7FE0; i < 0x7FF0; i++) rom[i] = 0;
  uint32_t sum = 0;
  for (size_t o = 0; o + 1 < rom.size(); o += 2)
    sum += static_cast<uint32_t>(rom[o] | (rom[o + 1] << 8));
  sum &= 0xFFFF;
  const uint32_t comp = (0x10000 - sum) & 0xFFFF;
  rom[0x7FE6] = static_cast<uint8_t>(sum & 0xFF);
  rom[0x7FE7] = static_cast<uint8_t>(sum >> 8);
  rom[0x7FE8] = static_cast<uint8_t>(comp & 0xFF);
  rom[0x7FE9] = static_cast<uint8_t>(comp >> 8);

  MasterSystemBus bus;
  CHECK(load(bus, rom));
  CHECK(bus.cartridge().mapper() == SegaCartridge::Mapper::Codemasters);
  bus.write(0x4000, 3);
  CHECK(bus.read(0x4000) == 0x23);
  bus.write(0x8000, 2);
  CHECK(bus.read(0x8000) == 0x22);
  bus.write(0x0000, 1);
  CHECK(bus.read(0x0000) == 0x21);  // No fixed first 1 KiB here.
  bus.write(0xFFFF, 0);             // Sega registers do nothing.
  CHECK(bus.read(0x8000) == 0x22);

  // The same layout with a wrong checksum is a plain Sega-mapper ROM.
  rom[0x7FE6] ^= 0xFF;
  MasterSystemBus plain;
  CHECK(load(plain, rom));
  CHECK(plain.cartridge().mapper() == SegaCartridge::Mapper::Sega);
}

void testPortsAndPads() {
  MasterSystemBus bus;
  CHECK(load(bus, pagedRom(2, 0x10)));

  // VDP through the ports: write a byte to VRAM, read it back.
  bus.writePort(0xBF, 0x10);
  bus.writePort(0xBF, 0x40 | 0x05);  // Write address 0x0510.
  bus.writePort(0xBE, 0xA5);
  bus.writePort(0xBF, 0x10);
  bus.writePort(0xBF, 0x05);  // Read address 0x0510.
  CHECK(bus.readPort(0xBE) == 0xA5);
  CHECK(bus.vdp().vram()[0x0510] == 0xA5);
  CHECK(bus.readPort(0xBF) == 0x1F);  // Status: no flags yet.
  // The block is mirrored: any odd port in 0x80-0xBF is control/status.
  bus.writePort(0xA1, 0x20);
  bus.writePort(0xA1, 0x40);
  bus.writePort(0x9E, 0x5A);
  CHECK(bus.vdp().vram()[0x0020] == 0x5A);

  // PSG: every write in 0x40-0x7F reaches it.
  bus.writePort(0x7F, 0x83);
  bus.writePort(0x7F, 0x12);
  CHECK(bus.psg().getState().tonePeriod[0] == 0x123);
  bus.writePort(0x41, 0x90);
  CHECK(bus.psg().getState().volume[0] == 0);

  // Counters: V counter on the even port, H counter on the odd one.
  CHECK(bus.readPort(0x7E) == 0);
  for (int i = 0; i < 400; i++) bus.step();
  CHECK(bus.readPort(0x7E) > 0);

  // Unused ports float high.
  CHECK(bus.readPort(0x10) == 0xFF);

  // Joypads are active low.
  CHECK(bus.readPort(0xDC) == 0xFF && bus.readPort(0xDD) == 0xFF);
  bus.setButton(0, MasterSystemBus::kUp, true);
  CHECK((bus.readPort(0xDC) & 0x01) == 0);
  bus.setButton(0, MasterSystemBus::kButton2, true);
  CHECK((bus.readPort(0xDC) & 0x20) == 0);
  bus.setButton(0, MasterSystemBus::kUp, false);
  CHECK((bus.readPort(0xDC) & 0x01) != 0);
  bus.setButton(1, MasterSystemBus::kUp, true);
  CHECK((bus.readPort(0xDC) & 0x40) == 0);
  bus.setButton(1, MasterSystemBus::kLeft, true);
  bus.setButton(1, MasterSystemBus::kButton1, true);
  CHECK((bus.readPort(0xDD) & 0x01) == 0);
  CHECK((bus.readPort(0xDD) & 0x04) == 0);
  CHECK(bus.readPort(0xC0) == bus.readPort(0xDC));  // Mirrors.
  CHECK(bus.readPort(0xC1) == bus.readPort(0xDD));
  bus.setButton(5, MasterSystemBus::kUp, true);  // Invalid player: ignored.

  // TH/TR lines configured as outputs read back their programmed level.
  bus.writePort(0x3F, 0x00);  // All outputs, level low.
  CHECK((bus.readPort(0xDC) & 0x20) == 0);
  CHECK((bus.readPort(0xDD) & 0xC8) == 0);
  bus.writePort(0x3F, 0xF0);  // All outputs, level high.
  CHECK((bus.readPort(0xDC) & 0x20) != 0);
  CHECK((bus.readPort(0xDD) & 0xC8) == 0xC8);
  bus.writePort(0x3F, 0xFF);  // Back to inputs.
  CHECK((bus.readPort(0xDD) & 0xC0) == 0xC0);
}

void testGameGearPorts() {
  MasterSystemBus gg(SegaVDP::Model::GameGear);
  CHECK(gg.isGameGear());
  CHECK(gg.readPort(0x00) == 0xFF);
  gg.setStart(true);
  CHECK(gg.readPort(0x00) == 0x7F);  // Start is bit 7, active low.
  gg.setStart(false);
  CHECK(gg.readPort(0x00) == 0xFF);
  gg.writePort(0x06, 0x01);
  CHECK(gg.psg().getState().stereo == 0x01);
  CHECK(gg.vdp().viewportWidth() == 160);

  MasterSystemBus sms;
  CHECK(!sms.isGameGear());
  CHECK(sms.vdp().viewportWidth() == 256);
}

void testFullMachine() {
  const std::vector<uint8_t> rom = demoRom();
  MasterSystemBus bus;
  CHECK(load(bus, rom));
  CHECK(bus.cpu().getPC() == 0);
  CHECK(bus.cpu().getState().SP == 0xDFF0);

  // The set-up code takes longer than the first scanlines, so check the
  // picture once a whole frame has been drawn with the final VDP state.
  runFrames(bus, 2);
  const auto &fb = bus.vdp().framebuffer();
  CHECK(fb[0] == kRed);              // Pixel (0,0) is tile 1, colour 1.
  CHECK(fb[7 + 7 * 256] == kRed);    // Bottom-right of the tile.
  CHECK(fb[8] == 0);                 // The next tile is empty.
  CHECK(bus.cpu().getState().IFF1);  // The program enabled interrupts.

  runFrames(bus, 3);
  const int count = bus.ram()[0];
  CHECK(count >= 4 && count <= 6);  // One interrupt per frame.

  // Frame timing: consecutive frames are 262 lines of 228 cycles apart.
  runFrames(bus, 1);
  const uint64_t t0 = bus.cpuCycles();
  runFrames(bus, 1);
  const long delta = static_cast<long>(bus.cpuCycles() - t0);
  CHECK(std::labs(delta - 262 * 228) < 30);

  MasterSystemBus pal;
  CHECK(load(pal, rom));
  pal.setPAL(true);
  runFrames(pal, 3);
  const uint64_t p0 = pal.cpuCycles();
  runFrames(pal, 1);
  CHECK(std::labs(static_cast<long>(pal.cpuCycles() - p0) - 313 * 228) < 30);

  // The PSG advances along with the CPU.
  bus.psg().clearSamples();
  runFrames(bus, 1);
  CHECK(bus.psg().frameCount() > 700 && bus.psg().frameCount() < 900);
}

void testPauseNmi() {
  MasterSystemBus bus;
  CHECK(load(bus, demoRom()));
  runFrames(bus, 2);
  CHECK(bus.ram()[1] == 0);
  bus.setPause(true);
  for (int i = 0; i < 10; i++) bus.step();
  CHECK(bus.ram()[1] == 1);  // NMI handler ran even though the CPU was halted.
  bus.setPause(true);        // Still held: no new edge.
  for (int i = 0; i < 10; i++) bus.step();
  CHECK(bus.ram()[1] == 1);
  bus.setPause(false);
  bus.setPause(true);
  for (int i = 0; i < 10; i++) bus.step();
  CHECK(bus.ram()[1] == 2);
}

void testSaveState() {
  const std::vector<uint8_t> rom = demoRom();
  MasterSystemBus a;
  CHECK(load(a, rom));
  a.setButton(0, MasterSystemBus::kLeft, true);
  runFrames(a, 3);

  nlohmann::json j = a.getState();
  MasterSystemBus b;
  CHECK(load(b, rom));
  b.setState(j.get<MasterSystemBusState>());
  CHECK(b.cpu().getPC() == a.cpu().getPC());
  CHECK(b.ram() == a.ram());
  CHECK((b.readPort(0xDC) & 0x04) == 0);  // Joypad state travels too.

  runFrames(a, 2);
  runFrames(b, 2);
  CHECK(a.cpu().getPC() == b.cpu().getPC());
  CHECK(a.cpuCycles() == b.cpuCycles());
  CHECK(a.ram() == b.ram());
  CHECK(a.vdp().framebuffer() == b.vdp().framebuffer());
}

}  // namespace

int main() {
  testMapperBanking();
  testCopierHeaderAndErrors();
  testCartridgeRam();
  testCodemasters();
  testPortsAndPads();
  testGameGearPorts();
  testFullMachine();
  testPauseNmi();
  testSaveState();
  if (failures == 0) std::printf("master_system_bus: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
