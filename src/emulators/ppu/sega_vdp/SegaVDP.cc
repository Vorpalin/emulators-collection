#include "emulators/ppu/sega_vdp/SegaVDP.hh"

#include <algorithm>

namespace {

inline uint8_t u8(int v) { return static_cast<uint8_t>(v); }
inline uint16_t u16(int v) { return static_cast<uint16_t>(v); }

constexpr uint8_t kScale2to5[4] = {0, 10, 21, 31};

}  // namespace

SegaVDP::SegaVDP(Model model) : gameGear_(model == Model::GameGear) { reset(); }

void SegaVDP::reset() {
  vram_.fill(0);
  cram_.fill(0);
  reg_.fill(0);
  reg_[10] = 0xFF;
  address_ = 0;
  code_ = 0;
  latchPending_ = false;
  latch_ = readBuffer_ = cramLatch_ = 0;
  vintPending_ = hintPending_ = false;
  spriteOverflow_ = spriteCollision_ = false;
  line_ = lineCycles_ = 0;
  hintCounter_ = reg_[10];
  vscroll_ = 0;
  irqLevel_ = false;
  fb_.fill(0);
}

void SegaVDP::setModel(Model model) { gameGear_ = model == Model::GameGear; }

SegaVDPState SegaVDP::getState() const {
  SegaVDPState s;
  s.vram = vram_;
  s.cram = cram_;
  s.reg = reg_;
  s.address = address_;
  s.code = code_;
  s.latchPending = latchPending_;
  s.latch = latch_;
  s.readBuffer = readBuffer_;
  s.cramLatch = cramLatch_;
  s.vintPending = vintPending_;
  s.hintPending = hintPending_;
  s.spriteOverflow = spriteOverflow_;
  s.spriteCollision = spriteCollision_;
  s.line = line_;
  s.lineCycles = lineCycles_;
  s.hintCounter = hintCounter_;
  s.vscroll = vscroll_;
  s.pal = pal_;
  s.gameGear = gameGear_;
  s.irqLevel = irqLevel_;
  return s;
}

void SegaVDP::setState(const SegaVDPState &s) {
  vram_ = s.vram;
  cram_ = s.cram;
  reg_ = s.reg;
  address_ = s.address;
  code_ = s.code;
  latchPending_ = s.latchPending;
  latch_ = s.latch;
  readBuffer_ = s.readBuffer;
  cramLatch_ = s.cramLatch;
  vintPending_ = s.vintPending;
  hintPending_ = s.hintPending;
  spriteOverflow_ = s.spriteOverflow;
  spriteCollision_ = s.spriteCollision;
  line_ = s.line;
  lineCycles_ = s.lineCycles;
  hintCounter_ = s.hintCounter;
  vscroll_ = s.vscroll;
  pal_ = s.pal;
  gameGear_ = s.gameGear;
  irqLevel_ = s.irqLevel;
}

// ---------------------------------------------------------------------------
// Ports
// ---------------------------------------------------------------------------

void SegaVDP::writeControl(uint8_t value) {
  if (!latchPending_) {
    latch_ = value;
    latchPending_ = true;
    address_ = u16((address_ & 0x3F00) | value);
    return;
  }
  latchPending_ = false;
  code_ = value >> 6;
  address_ = u16(((value & 0x3F) << 8) | latch_);
  if (code_ == 0) {  // VRAM read setup: prefetch
    readBuffer_ = vram_[address_];
    address_ = u16((address_ + 1) & 0x3FFF);
  } else if (code_ == 2) {  // Register write
    writeRegister(value & 0x0F, latch_);
  }
}

void SegaVDP::writeData(uint8_t value) {
  latchPending_ = false;
  if (code_ == 3) {
    writeCram(address_, value);
  } else {
    vram_[address_] = value;
  }
  readBuffer_ = value;
  address_ = u16((address_ + 1) & 0x3FFF);
}

uint8_t SegaVDP::readData() {
  latchPending_ = false;
  uint8_t result = readBuffer_;
  readBuffer_ = vram_[address_];
  address_ = u16((address_ + 1) & 0x3FFF);
  return result;
}

uint8_t SegaVDP::readStatus() {
  latchPending_ = false;
  uint8_t status = u8((vintPending_ ? 0x80 : 0) | (spriteOverflow_ ? 0x40 : 0) |
                      (spriteCollision_ ? 0x20 : 0) | 0x1F);
  vintPending_ = hintPending_ = false;
  spriteOverflow_ = spriteCollision_ = false;
  updateIrq();
  return status;
}

void SegaVDP::writeRegister(int index, uint8_t value) {
  reg_[static_cast<size_t>(index)] = value;
  updateIrq();  // Enabling an interrupt with a pending flag asserts /INT.
}

void SegaVDP::writeCram(uint16_t address, uint8_t value) {
  if (gameGear_) {
    // 12-bit entries: an even write latches, the following odd write commits.
    if (address & 1) {
      cram_[address & 0x3E] = cramLatch_;
      cram_[address & 0x3F] = value & 0x0F;
    } else {
      cramLatch_ = value;
    }
  } else {
    cram_[address & 0x1F] = value & 0x3F;
  }
}

uint16_t SegaVDP::paletteColor(int index) const {
  int r, g, b;
  if (gameGear_) {
    uint8_t lo = cram_[static_cast<size_t>(index * 2) & 0x3F];
    uint8_t hi = cram_[static_cast<size_t>(index * 2 + 1) & 0x3F];
    int r4 = lo & 0x0F, g4 = lo >> 4, b4 = hi & 0x0F;
    r = (r4 << 1) | (r4 >> 3);
    g = (g4 << 1) | (g4 >> 3);
    b = (b4 << 1) | (b4 >> 3);
  } else {
    uint8_t v = cram_[static_cast<size_t>(index) & 0x1F];
    r = kScale2to5[v & 3];
    g = kScale2to5[(v >> 2) & 3];
    b = kScale2to5[(v >> 4) & 3];
  }
  return u16(r | (g << 5) | (b << 10));
}

// ---------------------------------------------------------------------------
// Counters
// ---------------------------------------------------------------------------

int SegaVDP::activeHeight() const {
  if (gameGear_ || !(reg_[0] & 0x04)) return 192;
  const bool m2 = reg_[0] & 0x02;
  const bool m1 = reg_[1] & 0x10;
  const bool m3 = reg_[1] & 0x08;
  if (m2 && m1 && !m3) return 224;
  if (m2 && m3 && !m1) return 240;
  return 192;
}

uint8_t SegaVDP::readVCounter() const {
  const int h = activeHeight();
  int v = line_;
  if (!pal_) {
    const int jump = h == 192 ? 218 : 234;
    if (v > jump) v -= 6;
  } else {
    const int jump = h == 192 ? 242 : (h == 224 ? 258 : 266);
    if (v > jump) v -= 57;
  }
  return u8(v & 0xFF);
}

uint8_t SegaVDP::readHCounter() const {
  return u8((lineCycles_ * 342 / kCyclesPerLine) >> 1);
}

// ---------------------------------------------------------------------------
// Timing and interrupts
// ---------------------------------------------------------------------------

void SegaVDP::updateIrq() {
  bool level = (vintPending_ && (reg_[1] & 0x20)) ||
               (hintPending_ && (reg_[0] & 0x10));
  if (level != irqLevel_) {
    irqLevel_ = level;
    if (interruptCallback_) interruptCallback_(level);
  }
}

void SegaVDP::tick(uint32_t cpuCycles) {
  lineCycles_ += static_cast<int>(cpuCycles);
  while (lineCycles_ >= kCyclesPerLine) {
    lineCycles_ -= kCyclesPerLine;
    endLine();
  }
}

void SegaVDP::endLine() {
  const int height = activeHeight();
  if (line_ < height) renderLine(line_);

  line_++;
  if (line_ >= totalLines()) line_ = 0;
  if (line_ == 0) vscroll_ = reg_[9];

  // Line counter: runs on lines 0..height, reloaded on every other line.
  if (line_ <= height) {
    if (--hintCounter_ < 0) {
      hintCounter_ = reg_[10];
      hintPending_ = true;
    }
  } else {
    hintCounter_ = reg_[10];
  }

  if (line_ == height + 1) {
    vintPending_ = true;
    if (frameReadyCallback_) frameReadyCallback_();
  }
  updateIrq();
}

// ---------------------------------------------------------------------------
// Rendering (Mode 4)
// ---------------------------------------------------------------------------

void SegaVDP::renderLine(int y) {
  uint16_t *row = &fb_[static_cast<size_t>(y) * kWidth];

  std::array<uint16_t, 32> pal;
  for (int i = 0; i < 32; i++) pal[static_cast<size_t>(i)] = paletteColor(i);
  const uint16_t backdrop = pal[static_cast<size_t>(16 + (reg_[7] & 0x0F))];

  // Display off, or a legacy TMS9918 mode (unsupported): backdrop only.
  if (!(reg_[1] & 0x40) || !(reg_[0] & 0x04)) {
    std::fill(row, row + kWidth, backdrop);
    return;
  }

  const int height = activeHeight();
  const bool tall = height != 192;
  const int rowsPixels = tall ? 256 : 224;
  const int nameBase = tall ? (((reg_[2] & 0x0C) << 10) | 0x700)
                            : ((reg_[2] & 0x0E) << 10);
  const int hscroll = (y < 16 && (reg_[0] & 0x80)) ? 0 : reg_[8];

  // --- Background ---------------------------------------------------------
  std::array<uint8_t, kWidth> bgIndex;     // Colour index 0-15.
  std::array<bool, kWidth> bgPriority;
  std::array<uint8_t, kWidth> bgPalette;   // 0 or 16.
  for (int x = 0; x < kWidth; x++) {
    const int vs = (x >= 192 && (reg_[0] & 0x40)) ? 0 : vscroll_;
    const int srcY = (y + vs) % rowsPixels;
    const int srcX = (x - hscroll) & 0xFF;
    const int tileRow = srcY >> 3;
    int fineY = srcY & 7;
    const int tileCol = srcX >> 3;
    const int fineX = srcX & 7;

    const int addr = (nameBase + (tileRow * 32 + tileCol) * 2) & 0x3FFF;
    const int entry = vram_[static_cast<size_t>(addr)] |
                      (vram_[static_cast<size_t>((addr + 1) & 0x3FFF)] << 8);
    const int tile = entry & 0x1FF;
    const bool hflip = entry & 0x200;
    const bool vflip = entry & 0x400;
    if (vflip) fineY = 7 - fineY;

    const size_t t = static_cast<size_t>((tile * 32 + fineY * 4) & 0x3FFF);
    const int shift = hflip ? fineX : 7 - fineX;
    const int idx = ((vram_[t] >> shift) & 1) |
                    (((vram_[t + 1] >> shift) & 1) << 1) |
                    (((vram_[t + 2] >> shift) & 1) << 2) |
                    (((vram_[t + 3] >> shift) & 1) << 3);

    bgIndex[static_cast<size_t>(x)] = u8(idx);
    bgPriority[static_cast<size_t>(x)] = (entry & 0x1000) != 0;
    bgPalette[static_cast<size_t>(x)] = (entry & 0x0800) ? 16 : 0;
  }

  // --- Sprites ------------------------------------------------------------
  const int satBase = (reg_[5] & 0x7E) << 7;
  const bool tallSprites = reg_[1] & 0x02;
  const bool zoom = reg_[1] & 0x01;
  const int baseHeight = tallSprites ? 16 : 8;
  const int spriteHeight = zoom ? baseHeight * 2 : baseHeight;

  std::array<int, 8> list{};
  int found = 0;
  for (int i = 0; i < 64; i++) {
    const int sy = vram_[static_cast<size_t>((satBase + i) & 0x3FFF)];
    if (height == 192 && sy == 0xD0) break;
    int top = sy + 1;
    if (top > 240) top -= 256;
    if (y < top || y >= top + spriteHeight) continue;
    if (found == 8) {
      spriteOverflow_ = true;
      break;
    }
    list[static_cast<size_t>(found++)] = i;
  }

  std::array<uint8_t, kWidth> spriteColor{};  // 0 = none.
  std::array<bool, kWidth> spriteDrawn{};
  for (int n = 0; n < found; n++) {
    const int i = list[static_cast<size_t>(n)];
    const int sy = vram_[static_cast<size_t>((satBase + i) & 0x3FFF)];
    int top = sy + 1;
    if (top > 240) top -= 256;
    int x = vram_[static_cast<size_t>((satBase + 128 + i * 2) & 0x3FFF)];
    int tile = vram_[static_cast<size_t>((satBase + 129 + i * 2) & 0x3FFF)];
    if (reg_[0] & 0x08) x -= 8;
    if (reg_[6] & 0x04) tile |= 0x100;
    if (tallSprites) tile &= ~1;

    const int srcLine = zoom ? (y - top) >> 1 : (y - top);
    const size_t t = static_cast<size_t>((tile * 32 + srcLine * 4) & 0x3FFF);
    const int width = zoom ? 16 : 8;
    for (int px = 0; px < width; px++) {
      const int sx = x + px;
      if (sx < 0 || sx >= kWidth) continue;
      const int shift = 7 - (zoom ? px >> 1 : px);
      const int idx = ((vram_[t] >> shift) & 1) |
                      (((vram_[t + 1] >> shift) & 1) << 1) |
                      (((vram_[t + 2] >> shift) & 1) << 2) |
                      (((vram_[t + 3] >> shift) & 1) << 3);
      if (idx == 0) continue;
      if (spriteDrawn[static_cast<size_t>(sx)]) {
        spriteCollision_ = true;  // Overlap of two opaque sprite pixels.
        continue;
      }
      spriteDrawn[static_cast<size_t>(sx)] = true;
      spriteColor[static_cast<size_t>(sx)] = u8(idx);
    }
  }

  // --- Compose --------------------------------------------------------------
  const bool maskLeft = reg_[0] & 0x20;
  for (int x = 0; x < kWidth; x++) {
    const size_t xi = static_cast<size_t>(x);
    if (maskLeft && x < 8) {
      row[x] = backdrop;
      continue;
    }
    const bool bgOnTop = bgPriority[xi] && bgIndex[xi] != 0;
    if (spriteColor[xi] != 0 && !bgOnTop) {
      row[x] = pal[static_cast<size_t>(16 + spriteColor[xi])];
    } else {
      row[x] = pal[static_cast<size_t>(bgPalette[xi] + bgIndex[xi])];
    }
  }
}
