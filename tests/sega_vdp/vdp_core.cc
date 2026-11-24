// Sega VDP checks: port protocol, Mode 4 rendering (background, scroll,
// priority, sprites, masks), flags, interrupts, counters and the Game Gear
// palette. Everything is driven through the CPU-visible ports.

#include <cstdint>
#include <cstdio>
#include <vector>

#include "emulators/ppu/sega_vdp/SegaVDP.hh"

namespace {

int failures = 0;

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);       \
      failures++;                                                       \
    }                                                                   \
  } while (0)

constexpr uint16_t kRed = 31;            // RGB555 pure red.
constexpr uint16_t kGreen = 31 << 5;
constexpr uint16_t kBlue = 31 << 10;

void setReg(SegaVDP &v, int r, uint8_t value) {
  v.writeControl(value);
  v.writeControl(static_cast<uint8_t>(0x80 | r));
}

void setAddr(SegaVDP &v, uint16_t addr, int code) {
  v.writeControl(static_cast<uint8_t>(addr & 0xFF));
  v.writeControl(static_cast<uint8_t>((code << 6) | (addr >> 8)));
}

void writeVRAM(SegaVDP &v, uint16_t addr, const std::vector<uint8_t> &data) {
  setAddr(v, addr, 1);
  for (uint8_t b : data) v.writeData(b);
}

void writeCRAM(SegaVDP &v, uint16_t index, const std::vector<uint8_t> &data) {
  setAddr(v, index, 3);
  for (uint8_t b : data) v.writeData(b);
}

// Palette used by most tests: 0 black, 1 red, 2 green, 3 blue (both palettes).
void standardPalette(SegaVDP &v) {
  writeCRAM(v, 0, {0x00, 0x03, 0x0C, 0x30});
  writeCRAM(v, 16, {0x00, 0x03, 0x0C, 0x30});
}

// Mode 4, display on, name table at 0x3800, SAT at 0x3F00.
void basicMode(SegaVDP &v) {
  setReg(v, 0, 0x04);
  setReg(v, 1, 0x40);
  setReg(v, 2, 0xFF);
  setReg(v, 5, 0xFF);
}

// A tile whose every pixel has colour index `c` (planes replicated).
std::vector<uint8_t> solidTile(int c) {
  std::vector<uint8_t> t;
  for (int row = 0; row < 8; row++)
    for (int plane = 0; plane < 4; plane++) t.push_back((c >> plane) & 1 ? 0xFF : 0x00);
  return t;
}

void runLines(SegaVDP &v, int n) { v.tick(static_cast<uint32_t>(n * SegaVDP::kCyclesPerLine)); }

uint16_t px(const SegaVDP &v, int x, int y) {
  return v.framebuffer()[static_cast<size_t>(y) * SegaVDP::kWidth + static_cast<size_t>(x)];
}

void testPorts() {
  SegaVDP v;
  // VRAM write then read back through the prefetching read port.
  writeVRAM(v, 0x0123, {0xAA, 0xBB, 0xCC});
  setAddr(v, 0x0123, 0);
  CHECK(v.readData() == 0xAA);
  CHECK(v.readData() == 0xBB);
  CHECK(v.readData() == 0xCC);
  CHECK(v.vram()[0x0123] == 0xAA && v.vram()[0x0125] == 0xCC);

  // The address wraps at 16 KiB.
  writeVRAM(v, 0x3FFF, {0x11, 0x22});
  CHECK(v.vram()[0x3FFF] == 0x11 && v.vram()[0x0000] == 0x22);

  // Reading the status port resets a half-written control sequence.
  v.writeControl(0x55);
  v.readStatus();
  setAddr(v, 0x0123, 0);
  CHECK(v.readData() == 0xAA);

  // Register writes land in the register file (visible through behaviour).
  setReg(v, 1, 0x40);
  CHECK(v.activeHeight() == 192);
  setReg(v, 0, 0x06);
  setReg(v, 1, 0x50);  // M2 + M1 with mode 4: 224 lines.
  CHECK(v.activeHeight() == 224);
  setReg(v, 1, 0x48);  // M2 + M3: 240 lines.
  CHECK(v.activeHeight() == 240);
}

void testBackground() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  writeVRAM(v, 32, solidTile(1));  // Tile 1 = red.
  writeVRAM(v, 64, solidTile(2));  // Tile 2 = green.
  // Name table: (0,0)=tile 1, (1,0)=tile 2, (0,1)=tile 2 flipped irrelevant.
  writeVRAM(v, 0x3800, {1, 0, 2, 0});
  runLines(v, 8);
  CHECK(px(v, 0, 0) == kRed);
  CHECK(px(v, 7, 7) == kRed);
  CHECK(px(v, 8, 0) == kGreen);
  CHECK(px(v, 16, 0) == 0);   // Tile 0 is empty: palette colour 0.
  CHECK(px(v, 0, 8) == 0);
}

void testTileFlipsAndPalette() {
  SegaVDP v;
  standardPalette(v);
  writeCRAM(v, 17, {0x30});  // Sprite palette colour 1 = blue.
  basicMode(v);
  // Tile 1: left-most pixel set only in plane 0 of row 0.
  std::vector<uint8_t> t(32, 0);
  t[0] = 0x80;  // Row 0, plane 0, pixel x=0.
  writeVRAM(v, 32, t);
  // Entries: (0)=tile1, (1)=tile1+hflip, (2)=tile1+vflip, (3)=tile1+palette2.
  writeVRAM(v, 0x3800, {1, 0x00, 1, 0x02, 1, 0x04, 1, 0x08});
  runLines(v, 8);
  CHECK(px(v, 0, 0) == kRed);      // Normal.
  CHECK(px(v, 0, 7) == 0);
  CHECK(px(v, 8 + 7, 0) == kRed);  // H-flipped: pixel at the right edge.
  CHECK(px(v, 8, 0) == 0);
  CHECK(px(v, 16, 7) == kRed);     // V-flipped: pixel on the last row.
  CHECK(px(v, 16, 0) == 0);
  CHECK(px(v, 24, 0) == kBlue);    // Second palette selected by bit 11.
}

void testScrollAndMasks() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  writeVRAM(v, 32, solidTile(1));
  writeVRAM(v, 0x3800, {1, 0});  // Only the top-left tile is red.
  setReg(v, 8, 8);               // Scroll the picture 8 pixels to the right.
  runLines(v, 1);
  CHECK(px(v, 0, 0) == 0);
  CHECK(px(v, 8, 0) == kRed);
  CHECK(px(v, 15, 0) == kRed);
  CHECK(px(v, 16, 0) == 0);

  // R0 bit 7 locks the horizontal scroll for the first two tile rows.
  setReg(v, 0, 0x84);
  runLines(v, 261);  // Complete the frame, back to line 0.
  runLines(v, 1);
  CHECK(px(v, 0, 0) == kRed);

  // Left column mask: the first 8 pixels show the backdrop (sprite pal 0).
  setReg(v, 0, 0x24);
  setReg(v, 7, 0x02);  // Backdrop = sprite palette colour 2 = green.
  runLines(v, 261);
  runLines(v, 1);
  CHECK(px(v, 0, 0) == kGreen);
  CHECK(px(v, 7, 0) == kGreen);
}

void testVerticalScroll() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  writeVRAM(v, 32, solidTile(1));
  writeVRAM(v, 0x3800 + 64, {1, 0});  // Tile row 1, column 0 is red.
  CHECK(v.vram()[0x3800 + 64] == 1);
  setReg(v, 9, 8);  // Scroll up by 8 pixels; latched when a frame starts.
  runLines(v, 262);  // First frame (old scroll), back to line 0.
  runLines(v, 8);
  CHECK(px(v, 0, 0) == kRed);   // Tile row 1 now appears at the top.
  CHECK(px(v, 7, 7) == kRed);

  // R0 bit 6 locks vertical scroll for the right-most 8 columns (x >= 192).
  setReg(v, 0, 0x44);
  runLines(v, 254);
  runLines(v, 8);
  CHECK(px(v, 0, 0) == kRed);   // Scrolled column still shifted.
  writeVRAM(v, 0x3800 + 24 * 2, {1, 0});  // Row 0, column 24 (x = 192).
  runLines(v, 254);
  runLines(v, 8);
  CHECK(px(v, 192, 0) == kRed);  // Locked column ignores the scroll.
}

void testSprites() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  writeVRAM(v, 64, solidTile(3));  // Sprite tile 2 = colour 3 (blue, pal 1).
  // Sprite 0 at x=100, y=19 (appears on line 20); terminator after it.
  writeVRAM(v, 0x3F00, {19, 0xD0});
  writeVRAM(v, 0x3F80, {100, 2});
  runLines(v, 29);
  CHECK(px(v, 100, 19) == 0);       // Y byte + 1 = first visible line (20).
  CHECK(px(v, 100, 20) == kBlue);
  CHECK(px(v, 107, 27) == kBlue);
  CHECK(px(v, 108, 20) == 0);
  CHECK(px(v, 99, 20) == 0);

  // Sprite shift (R0 bit 3) moves sprites 8 pixels to the left.
  setReg(v, 0, 0x0C);
  runLines(v, 262 - 29 + 21);  // Next frame, past line 20.
  CHECK(px(v, 92, 20) == kBlue);
  CHECK(px(v, 100, 20) == 0);
}

void testSpritePriorityAndFlags() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  writeVRAM(v, 32, solidTile(1));   // BG tile 1: red.
  writeVRAM(v, 64, solidTile(3));   // Sprite tile 2: blue.
  // Background tile 1 with the priority bit at (0,0); tile 1 without at (1,0).
  writeVRAM(v, 0x3800, {1, 0x10, 1, 0x00});
  writeVRAM(v, 0x3F00, {255, 255, 0xD0});  // Sprites 0,1 on line 0 (y=255 -> 0).
  writeVRAM(v, 0x3F80, {0, 2, 8, 2});
  runLines(v, 1);
  CHECK(px(v, 0, 0) == kRed);   // BG priority wins over the sprite.
  CHECK(px(v, 8, 0) == kBlue);  // Sprite drawn over a normal tile.
  CHECK(v.readStatus() == 0x1F);  // No collision, no overflow.

  // Collision: two sprites overlapping.
  SegaVDP c;
  standardPalette(c);
  basicMode(c);
  writeVRAM(c, 64, solidTile(3));
  writeVRAM(c, 0x3F00, {9, 9, 0xD0});
  writeVRAM(c, 0x3F80, {50, 2, 54, 2});
  runLines(c, 11);
  CHECK((c.readStatus() & 0x20) != 0);
  CHECK((c.readStatus() & 0x20) == 0);  // Cleared by the read.

  // Overflow: nine sprites on one line.
  SegaVDP o;
  standardPalette(o);
  basicMode(o);
  writeVRAM(o, 64, solidTile(3));
  std::vector<uint8_t> ys(9, 9), xn;
  ys.push_back(0xD0);
  for (int i = 0; i < 9; i++) {
    xn.push_back(static_cast<uint8_t>(i * 16));
    xn.push_back(2);
  }
  writeVRAM(o, 0x3F00, ys);
  writeVRAM(o, 0x3F80, xn);
  runLines(o, 11);
  CHECK((o.readStatus() & 0x40) != 0);
  // Only the first eight are drawn: the ninth (x = 128) is missing.
  CHECK(px(o, 7 * 16, 10) == kBlue);
  CHECK(px(o, 8 * 16, 10) == 0);
}

void testTallAndZoomedSprites() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  setReg(v, 1, 0x42);  // 8x16 sprites.
  writeVRAM(v, 64, solidTile(3));   // Tile 2: top half.
  writeVRAM(v, 96, solidTile(1));   // Tile 3: bottom half (red).
  writeVRAM(v, 0x3F00, {0, 0xD0});
  writeVRAM(v, 0x3F80, {10, 3});  // Odd tile index: bit 0 is ignored (2/3).
  runLines(v, 18);
  CHECK(px(v, 10, 1) == kBlue);
  CHECK(px(v, 10, 8) == kBlue);
  CHECK(px(v, 10, 9) == kRed);
  CHECK(px(v, 10, 16) == kRed);
  CHECK(px(v, 10, 17) == 0);

  SegaVDP z;
  standardPalette(z);
  basicMode(z);
  setReg(z, 1, 0x41);  // Zoomed 8x8 -> 16x16.
  writeVRAM(z, 64, solidTile(3));
  writeVRAM(z, 0x3F00, {0, 0xD0});
  writeVRAM(z, 0x3F80, {10, 2});
  runLines(z, 17);
  CHECK(px(z, 25, 16) == kBlue);
  CHECK(px(z, 26, 16) == 0);
  CHECK(px(z, 10, 17) == 0);
}

void testInterrupts() {
  SegaVDP v;
  basicMode(v);
  bool level = false;
  int frames = 0, edges = 0;
  v.onInterrupt([&](bool l) {
    level = l;
    if (l) edges++;
  });
  v.onFrameReady([&] { frames++; });

  setReg(v, 1, 0x60);  // Display + frame interrupt enable.
  runLines(v, 192);
  CHECK(!level && frames == 0);
  runLines(v, 1);  // Start of line 193: VINT.
  CHECK(level && frames == 1 && v.irqLine());
  CHECK((v.readStatus() & 0x80) != 0);
  CHECK(!level);  // Reading the status register acknowledges it.

  // With the interrupt disabled the flag is still set but /INT stays low.
  SegaVDP w;
  basicMode(w);
  runLines(w, 193);
  CHECK(!w.irqLine());
  setReg(w, 1, 0x60);  // Enabling it with the flag pending asserts /INT.
  CHECK(w.irqLine());

  // Line interrupt: one every R10 + 1 lines, counted over a whole frame.
  SegaVDP l;
  basicMode(l);
  setReg(l, 0, 0x14);  // Line interrupt enable.
  setReg(l, 10, 9);
  int lineIrqs = 0;
  l.onInterrupt([&](bool lv) {
    if (lv) lineIrqs++;
  });
  runLines(l, 262);  // Settle: the first frame starts from reset state.
  lineIrqs = 0;
  for (int i = 0; i < 262; i++) {
    runLines(l, 1);
    l.readStatus();  // Acknowledge so that every new event is a fresh edge.
  }
  CHECK(lineIrqs == 19);  // Lines 9, 19, ..., 189.
  (void)edges;
}

void testCounters() {
  SegaVDP v;
  runLines(v, 100);
  CHECK(v.readVCounter() == 100);
  runLines(v, 118);  // Line 218: last linear value in NTSC/192.
  CHECK(v.readVCounter() == 0xDA);
  runLines(v, 1);    // Line 219 jumps back to 0xD5.
  CHECK(v.readVCounter() == 0xD5);
  runLines(v, 42);   // Line 261: last line.
  CHECK(v.readVCounter() == 0xFF);

  SegaVDP p;
  p.setPAL(true);
  runLines(p, 243);
  CHECK(p.readVCounter() == 0xBA);
  runLines(p, 69);
  CHECK(p.readVCounter() == 0xFF);
}

void testDisplayOffAndHeight() {
  SegaVDP v;
  standardPalette(v);
  setReg(v, 0, 0x04);
  setReg(v, 7, 0x01);  // Backdrop: sprite palette colour 1.
  setReg(v, 1, 0x00);  // Display off.
  runLines(v, 1);
  CHECK(px(v, 0, 0) == kRed);
  CHECK(px(v, 255, 0) == kRed);
}

void testGameGear() {
  SegaVDP g(SegaVDP::Model::GameGear);
  CHECK(g.viewportX() == 48 && g.viewportY() == 24);
  CHECK(g.viewportWidth() == 160 && g.viewportHeight() == 144);
  // 12-bit colour: entry 1 = R 0xF, G 0x0, B 0x0 -> bytes 0x0F, 0x00.
  writeCRAM(g, 0, {0x00, 0x00, 0x0F, 0x00});
  writeCRAM(g, 32, {0x00, 0x00, 0x0F, 0x00});
  basicMode(g);
  writeVRAM(g, 32, solidTile(1));
  writeVRAM(g, 0x3800, {1, 0});
  runLines(g, 1);
  CHECK(px(g, 0, 0) == kRed);

  // Green and blue nibbles: entry 2 = 0x0F0 (G) / 0x00F (B high byte).
  writeCRAM(g, 4, {0xF0, 0x00, 0x00, 0x0F});
  writeVRAM(g, 64, solidTile(2));
  writeVRAM(g, 96, solidTile(3));
  writeVRAM(g, 0x3802, {2, 0, 3, 0});
  runLines(g, 262);
  runLines(g, 1);
  CHECK(px(g, 8, 0) == kGreen);
  CHECK(px(g, 16, 0) == kBlue);
}

void testState() {
  SegaVDP v;
  standardPalette(v);
  basicMode(v);
  writeVRAM(v, 32, solidTile(1));
  runLines(v, 50);
  SegaVDPState s = v.getState();
  nlohmann::json j = s;
  SegaVDPState r = j.get<SegaVDPState>();
  CHECK(r.vram == s.vram && r.cram == s.cram && r.reg == s.reg);
  CHECK(r.line == s.line && r.lineCycles == s.lineCycles);
  SegaVDP w;
  w.setState(r);
  CHECK(w.currentLine() == 50);
}

}  // namespace

int main() {
  testPorts();
  testBackground();
  testTileFlipsAndPalette();
  testScrollAndMasks();
  testVerticalScroll();
  testSprites();
  testSpritePriorityAndFlags();
  testTallAndZoomedSprites();
  testInterrupts();
  testCounters();
  testDisplayOffAndHeight();
  testGameGear();
  testState();
  if (failures == 0) std::printf("sega_vdp: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
