// Focused PPU regression tests using direct register/memory access.
// No external ROMs are required.
#include <cstdint>
#include <cstdio>

#include "emulators/ppu/game_boy_ppu/GameBoyPPU.hh"

namespace {

int failures = 0;

void expect(const char* name, int actual, int expected) {
  if (actual != expected) {
    std::printf("FAIL %-42s got %02X, expected %02X\n", name, actual,
                expected);
    ++failures;
  } else {
    std::printf("ok   %-42s %02X\n", name, actual);
  }
}

void setMode(GameBoyPPU& ppu, Mode mode, bool lcdEnabled = true) {
  auto state = ppu.getState();
  state.regs.lcdc = lcdEnabled ? 0x80 : 0x00;
  state.regs.stat = static_cast<uint8_t>(
      (state.regs.stat & 0xFC) | static_cast<uint8_t>(mode));
  ppu.setState(state);
}

void testMemoryAccessLocks() {
  GameBoyPPU ppu;
  ppu.setCgbMode(true);

  setMode(ppu, OAMSearch);
  ppu.write(0x8000, 0x12);
  expect("Mode 2: VRAM remains accessible", ppu.read(0x8000), 0x12);
  ppu.write(0xFE00, 0x34);
  expect("Mode 2: OAM reads are blocked", ppu.read(0xFE00), 0xFF);

  setMode(ppu, PixelTransfer);
  expect("Mode 3: VRAM reads are blocked", ppu.read(0x8000), 0xFF);
  expect("Mode 3: OAM reads are blocked", ppu.read(0xFE00), 0xFF);
  ppu.write(0x8000, 0x56);
  ppu.write(0xFE00, 0x78);

  // DMA masters bypass the CPU-facing locks.
  ppu.writeVRAMDMA(0x0000, 0x9A);
  ppu.writeOAMDMA(0x00, 0xBC);

  setMode(ppu, HBlank);
  expect("Mode 0: VRAM CPU write stayed blocked", ppu.read(0x8000), 0x9A);
  expect("Mode 0: OAM CPU write stayed blocked", ppu.read(0xFE00), 0xBC);
  ppu.write(0x8000, 0xDE);
  ppu.write(0xFE00, 0xF0);
  expect("Mode 0: VRAM is accessible", ppu.read(0x8000), 0xDE);
  expect("Mode 0: OAM is accessible", ppu.read(0xFE00), 0xF0);

  setMode(ppu, PixelTransfer, false);
  ppu.write(0x8000, 0x22);
  ppu.write(0xFE00, 0x33);
  expect("LCD off: VRAM is accessible", ppu.read(0x8000), 0x22);
  expect("LCD off: OAM is accessible", ppu.read(0xFE00), 0x33);
}

void testStatRegisterWriteMask() {
  GameBoyPPU ppu;
  auto state = ppu.getState();
  state.regs.lcdc = 0x80;
  state.regs.stat = static_cast<uint8_t>(
      0x04 | static_cast<uint8_t>(OAMSearch));  // coincidence + Mode 2.
  ppu.setState(state);

  ppu.write(0xFF41, 0x00);
  expect("STAT write preserves mode and coincidence bits",
         ppu.read(0xFF41), 0x86);
  ppu.write(0xFF41, 0x78);
  expect("STAT write updates only interrupt-enable bits",
         ppu.read(0xFF41), 0xFE);
}

void testCgbPaletteAccessLocks() {
  GameBoyPPU ppu;
  auto state = ppu.getState();
  state.cgbMode = true;
  state.regs.lcdc = 0x80;
  state.regs.stat = static_cast<uint8_t>(
      (state.regs.stat & 0xFC) | static_cast<uint8_t>(PixelTransfer));
  state.bgPalette[0] = 0x12;
  state.bgPalette[1] = 0x34;
  ppu.setState(state);

  ppu.write(0xFF68, 0x80);  // BG palette index 0, auto-increment enabled.
  expect("Mode 3: BG palette reads are blocked", ppu.read(0xFF69), 0xFF);
  ppu.write(0xFF69, 0x00);  // Data write is ignored, index still increments.
  expect("Mode 3: blocked BG palette write increments index",
         ppu.read(0xFF68), 0xC1);

  setMode(ppu, HBlank);
  ppu.write(0xFF68, 0x00);
  expect("Mode 0: blocked palette write preserved CRAM",
         ppu.read(0xFF69), 0x12);
}

void prepareDmgLine(GameBoyPPU& ppu) {
  auto state = ppu.getState();
  state.regs.lcdc = 0x93;  // LCD on, unsigned tile data, OBJ and BG enabled.
  state.regs.stat = static_cast<uint8_t>(
      (state.regs.stat & 0xFC) | static_cast<uint8_t>(OAMSearch));
  state.regs.ly = 0;
  state.regs.bgp = 0x00;
  state.vram.fill(0);
  state.oam.fill(0);
  state.vram[0x1800] = 0;  // BG map points to tile 0.
  ppu.setState(state);
}

void renderFirstLine(GameBoyPPU& ppu) {
  ppu.tick(80);   // OAM search -> pixel transfer.
  ppu.tick(172);  // Pixel transfer -> HBlank; renders the line.
}

void testDmgSpritePriorityUsesRawBgIndex() {
  GameBoyPPU ppu;
  prepareDmgLine(ppu);

  auto state = ppu.getState();
  state.regs.bgp = 0x00;   // Every BG colour maps to shade 0.
  state.regs.obp0 = 0x0C;  // OBJ colour 1 maps to shade 3.
  state.vram[0] = 0xFF;    // BG tile 0: raw colour index 1.
  state.vram[16] = 0xFF;   // OBJ tile 1: raw colour index 1.
  state.oam[0] = 16;
  state.oam[1] = 8;
  state.oam[2] = 1;
  state.oam[3] = 0x80;     // Behind non-zero BG pixels.
  ppu.setState(state);

  renderFirstLine(ppu);
  expect("DMG OBJ-behind-BG uses raw BG colour index",
         ppu.framebuffer()[0], 0);
}

void testHigherPriorityHiddenSpriteBlocksLowerPrioritySprite() {
  GameBoyPPU ppu;
  prepareDmgLine(ppu);

  auto state = ppu.getState();
  state.regs.bgp = 0x04;   // BG colour 1 -> shade 1.
  state.regs.obp0 = 0x0C;  // Higher-priority OBJ colour 1 -> shade 3.
  state.regs.obp1 = 0x08;  // Lower-priority OBJ colour 1 -> shade 2.
  state.vram[0] = 0xFF;    // BG tile 0: colour index 1.
  state.vram[16] = 0xFF;   // OBJ tile 1: colour index 1.
  state.vram[32] = 0xFF;   // OBJ tile 2: colour index 1.

  // The first sprite has priority but is behind the BG. The second sprite
  // must not show through it at the same pixel.
  state.oam[0] = 16;
  state.oam[1] = 8;
  state.oam[2] = 1;
  state.oam[3] = 0x80;
  state.oam[4] = 16;
  state.oam[5] = 8;
  state.oam[6] = 2;
  state.oam[7] = 0x10;  // OBP1, in front of BG.
  ppu.setState(state);

  renderFirstLine(ppu);
  expect("hidden high-priority OBJ blocks lower OBJ",
         ppu.framebuffer()[0], 1);
}

void testDmgEqualXUsesOamOrder() {
  GameBoyPPU ppu;
  prepareDmgLine(ppu);

  auto state = ppu.getState();
  state.regs.obp0 = 0x04;  // OBJ colour 1 -> shade 1.
  state.regs.obp1 = 0x08;  // OBJ colour 1 -> shade 2.
  state.vram[16] = 0xFF;   // Tile 1: colour index 1.
  state.vram[32] = 0xFF;   // Tile 2: colour index 1.

  // Same position: lower OAM index must win on DMG.
  state.oam[0] = 16;
  state.oam[1] = 8;
  state.oam[2] = 1;
  state.oam[3] = 0x00;  // OBP0.
  state.oam[4] = 16;
  state.oam[5] = 8;
  state.oam[6] = 2;
  state.oam[7] = 0x10;  // OBP1.
  ppu.setState(state);

  renderFirstLine(ppu);
  expect("DMG equal-X sprite priority follows OAM order",
         ppu.framebuffer()[0], 1);
}

}  // namespace

int main() {
  testMemoryAccessLocks();
  testStatRegisterWriteMask();
  testCgbPaletteAccessLocks();
  testDmgSpritePriorityUsesRawBgIndex();
  testHigherPriorityHiddenSpriteBlocksLowerPrioritySprite();
  testDmgEqualXUsesOamOrder();

  std::printf("%s (%d failure%s)\n", failures ? "FAILED" : "PASSED",
              failures, failures == 1 ? "" : "s");
  return failures ? 1 : 0;
}
