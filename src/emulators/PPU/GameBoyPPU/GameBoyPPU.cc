#include "GameBoyPPU.hh"

#include <algorithm>

constexpr int CYCLES_OAM_SCAN = 80;
constexpr int CYCLES_DRAWING = 172;
constexpr int CYCLES_LINE = 456;
constexpr int LINES_VBLANK_END = 153;

GameBoyPPU::GameBoyPPU() { reset(); }

void GameBoyPPU::reset() {
  regs_ = PPURegisters{};
  vram.fill(0);
  oam.fill(0);
  framebuffer_.fill(0);
  dotCounter_ = 0;
}

void GameBoyPPU::tick(uint8_t cycles) {
  if (!regs_.lcdEnabled()) {
    // If LCD is disabled, reset the PPU state
    regs_.setMode(HBlank);
    regs_.ly = 0;
    dotCounter_ = 0;
    return;
  }
  dotCounter_ += cycles;

  switch (regs_.mode()) {
    case OAMSearch:
      if (dotCounter_ >= CYCLES_OAM_SCAN) {
        dotCounter_ -= CYCLES_OAM_SCAN;
        changeMode(PixelTransfer);
      }
      break;
    case PixelTransfer:
      if (dotCounter_ >= CYCLES_DRAWING) {
        dotCounter_ -= CYCLES_DRAWING;
        renderScanline();
        changeMode(HBlank);
      }
      break;
    case HBlank:
      if (dotCounter_ >= CYCLES_LINE - CYCLES_OAM_SCAN - CYCLES_DRAWING) {
        dotCounter_ -= (CYCLES_LINE - CYCLES_OAM_SCAN - CYCLES_DRAWING);
        regs_.ly++;
        checkLYC();
        if (regs_.ly == SCREEN_HEIGHT) {
          changeMode(VBlank);
          if (vblankInterrupt_) vblankInterrupt_();
          if (frameReadyCallback_) frameReadyCallback_();
        } else {
          changeMode(OAMSearch);
        }
      }
      break;
    case VBlank:
      if (dotCounter_ >= CYCLES_LINE) {
        dotCounter_ -= CYCLES_LINE;
        regs_.ly++;
        checkLYC();
        if (regs_.ly > LINES_VBLANK_END) {
          regs_.ly = 0;
          changeMode(OAMSearch);
        }
      }
      break;
  }
}

void GameBoyPPU::changeMode(Mode newMode) {
  regs_.setMode(newMode);
  bool interruptTriggered = false;
  switch (newMode) {
    case HBlank:
      if (regs_.hblankInterruptEnabled()) {
        interruptTriggered = true;
      }
      break;
    case VBlank:
      if (regs_.vblankInterruptEnabled()) {
        interruptTriggered = true;
      }
      break;
    case OAMSearch:
      if (regs_.oamInterruptEnabled()) {
        interruptTriggered = true;
      }
      break;
    case PixelTransfer:
      // No STAT interrupt for Pixel Transfer mode
      break;
  }

  if (interruptTriggered && statInterrupt_) {
    statInterrupt_();
  }
}

void GameBoyPPU::checkLYC() {
  if (regs_.ly == regs_.lyc) {
    regs_.stat |= 0x04;  // Set coincidence flag
    if (regs_.lycInterruptEnabled() && statInterrupt_) {
      statInterrupt_();
    }
  } else {
    regs_.stat &= ~0x04;  // Clear coincidence flag
  }
}

void GameBoyPPU::renderScanline() {
  int line = regs_.ly;
  if (line < SCREEN_HEIGHT) {
    if (regs_.bgWindowEnabled()) {
      renderBackgroundLine(line);
      if (regs_.windowEnabled()) {
        renderWindowLine(line);
      }
    } else {
      // disable background and window rendering, fill the line with white
      // (color 0)
      for (int x = 0; x < SCREEN_WIDTH; x++) {
        framebuffer_[line * SCREEN_WIDTH + x] = 0;
      }
    }

    if (regs_.spritesEnabled()) {
      renderSpritesLine(line);
    }
  }
}

void GameBoyPPU::renderBackgroundLine(int line) {
  uint16_t tileMapBase = regs_.bgTileMapHigh() ? 0x9C00 : 0x9800;

  uint8_t scrolledY = static_cast<uint8_t>(line + regs_.scy);
  int tileRow = scrolledY / 8;
  int pixelRowInTile = scrolledY % 8;

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    uint8_t scrolledX = static_cast<uint8_t>(x + regs_.scx);
    int tileCol = scrolledX / 8;
    int pixelColInTile = scrolledX % 8;

    uint16_t tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
    uint8_t tileIndex = readVRAM(tileMapAddr);

    uint16_t tileDataAddr;
    if (regs_.bgWindowDataLow()) {
      // Mode non signé : 0x8000 + index * 16
      tileDataAddr = 0x8000 + (static_cast<uint16_t>(tileIndex) * 16);
    } else {
      // Mode signé : 0x9000 + (index signé) * 16
      int8_t signedIndex = static_cast<int8_t>(tileIndex);
      tileDataAddr = static_cast<uint16_t>(0x9000 + (signedIndex * 16));
    }

    uint8_t colorIndex =
        readTilePixel(tileDataAddr, pixelColInTile, pixelRowInTile);
    uint8_t shade = getColorFromPalette(regs_.bgp, colorIndex);

    framebuffer_[line * SCREEN_WIDTH + x] = shade;
  }
}

void GameBoyPPU::renderWindowLine(int line) {
  if (line < regs_.wy) return;

  int windowX = static_cast<int>(regs_.wx) - 7;
  if (windowX >= SCREEN_WIDTH) return;

  uint16_t tileMapBase = regs_.windowTileMapHigh() ? 0x9C00 : 0x9800;

  int windowLine = line - regs_.wy;
  int tileRow = windowLine / 8;
  int pixelRowInTile = windowLine % 8;

  for (int x = std::max(0, windowX); x < SCREEN_WIDTH; x++) {
    int winPixelX = x - windowX;
    int tileCol = winPixelX / 8;
    int pixelColInTile = winPixelX % 8;

    uint16_t tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
    uint8_t tileIndex = readVRAM(tileMapAddr);

    uint16_t tileDataAddr;
    if (regs_.bgWindowDataLow()) {
      tileDataAddr = 0x8000 + (static_cast<uint16_t>(tileIndex) * 16);
    } else {
      int8_t signedIndex = static_cast<int8_t>(tileIndex);
      tileDataAddr = static_cast<uint16_t>(0x9000 + (signedIndex * 16));
    }

    uint8_t colorIndex =
        readTilePixel(tileDataAddr, pixelColInTile, pixelRowInTile);
    uint8_t shade = getColorFromPalette(regs_.bgp, colorIndex);

    framebuffer_[line * SCREEN_WIDTH + x] = shade;
  }
}

void GameBoyPPU::renderSpritesLine(int line) {
  int spriteHeight = regs_.spriteSize8x16() ? 16 : 8;

  std::vector<Sprite> visibleSprites;
  visibleSprites.reserve(10);

  for (int i = 0; i < 40 && visibleSprites.size() < 10; i++) {
    const uint8_t* base = &oam[i * 4];
    Sprite s{base[0], base[1], base[2], base[3]};

    int spriteScreenY = static_cast<int>(s.y) - 16;
    if (line >= spriteScreenY && line < spriteScreenY + spriteHeight) {
      visibleSprites.push_back(s);
    }
  }

  // Priorité : les sprites avec un X plus petit sont dessinés par-dessus
  // (on les traite donc en dernier dans la boucle, du plus grand X au plus
  // petit)
  std::stable_sort(visibleSprites.begin(), visibleSprites.end(),
                   [](const Sprite& a, const Sprite& b) { return a.x > b.x; });

  for (const auto& s : visibleSprites) {
    int spriteScreenY = static_cast<int>(s.y) - 16;
    int spriteScreenX = static_cast<int>(s.x) - 8;

    int rowInSprite = line - spriteScreenY;
    if (s.flipY()) rowInSprite = spriteHeight - 1 - rowInSprite;

    uint8_t tileIndex = s.tileIndex;
    if (spriteHeight == 16) tileIndex &= 0xFE;  // ignore le bit 0 en mode 8x16

    uint16_t tileDataAddr = 0x8000 + (static_cast<uint16_t>(tileIndex) * 16);

    for (int col = 0; col < 8; col++) {
      int screenX = spriteScreenX + col;
      if (screenX < 0 || screenX >= SCREEN_WIDTH) continue;

      int colInSprite = s.flipX() ? (7 - col) : col;
      uint8_t colorIndex =
          readTilePixel(tileDataAddr, colInSprite, rowInSprite);

      if (colorIndex == 0) continue;  // 0 = transparent pour les sprites

      if (s.priorityBehindBG()) {
        uint8_t bgShade = framebuffer_[line * SCREEN_WIDTH + screenX];
        uint8_t bgColor0 = getColorFromPalette(regs_.bgp, 0);
        if (bgShade != bgColor0)
          continue;  // le fond passe devant sauf s'il est "blanc"
      }

      uint8_t palette = s.paletteOBP1() ? regs_.obp1 : regs_.obp0;
      uint8_t shade = getColorFromPalette(palette, colorIndex);

      framebuffer_[line * SCREEN_WIDTH + screenX] = shade;
    }
  }
}

void GameBoyPPU::writeVRAM(uint16_t addr, uint8_t value) {
  if (addr < 0x2000) {
    vram[addr] = value;
  }
}

void GameBoyPPU::writeOAM(uint16_t addr, uint8_t value) {
  if (addr < 0xA0) {
    oam[addr] = value;
  }
}

void GameBoyPPU::writeRegister(uint16_t addr, uint8_t value) {
  switch (addr) {
    case 0xFF40:
      regs_.lcdc = value;
      break;
    case 0xFF41:
      regs_.stat = value;
      break;
    case 0xFF42:
      regs_.scy = value;
      break;
    case 0xFF43:
      regs_.scx = value;
      break;
    case 0xFF44: /* LY is read-only */
      break;
    case 0xFF45:
      regs_.lyc = value;
      checkLYC();
      break;
    case 0xFF46:
      regs_.dma = value; /* DMA transfer handled elsewhere */
      break;
    case 0xFF47:
      regs_.bgp = value;
      break;
    case 0xFF48:
      regs_.obp0 = value;
      break;
    case 0xFF49:
      regs_.obp1 = value;
      break;
    case 0xFF4A:
      regs_.wy = value;
      break;
    case 0xFF4B:
      regs_.wx = value;
      break;
  }
}

void GameBoyPPU::writeMemory(uint16_t address, uint8_t value) {
  if (address >= 0x8000 && address <= 0x9FFF) {
    writeVRAM(address - 0x8000, value);
  } else if (address >= 0xFE00 && address <= 0xFE9F) {
    writeOAM(address - 0xFE00, value);
  } else if (address >= 0xFF40 && address <= 0xFF4B) {
    writeRegister(address, value);
  }
}

uint8_t GameBoyPPU::readVRAM(uint16_t addr) const {
  if (addr < 0x2000) {
    return vram[addr];
  }
  return 0xFF;  // Invalid read
}

uint8_t GameBoyPPU::readOAM(uint16_t addr) const {
  if (addr < 0xA0) {
    return oam[addr];
  }
  return 0xFF;  // Invalid read
}

uint8_t GameBoyPPU::readMemory(uint16_t address) {
  if (address >= 0x8000 && address <= 0x9FFF) {
    return readVRAM(address - 0x8000);
  } else if (address >= 0xFE00 && address <= 0xFE9F) {
    return readOAM(address - 0xFE00);
  } else if (address >= 0xFF40 && address <= 0xFF4B) {
    return readRegister(address);
  }
  return 0xFF;  // Invalid read
}

uint8_t GameBoyPPU::readRegister(uint16_t addr) const {
  switch (addr) {
    case 0xFF40:
      return regs_.lcdc;
    case 0xFF41:
      return regs_.stat;
    case 0xFF42:
      return regs_.scy;
    case 0xFF43:
      return regs_.scx;
    case 0xFF44:
      return regs_.ly;
    case 0xFF45:
      return regs_.lyc;
    case 0xFF46:
      return regs_.dma;
    case 0xFF47:
      return regs_.bgp;
    case 0xFF48:
      return regs_.obp0;
    case 0xFF49:
      return regs_.obp1;
    case 0xFF4A:
      return regs_.wy;
    case 0xFF4B:
      return regs_.wx;
  }
  return 0xFF;  // Invalid read
}

uint8_t GameBoyPPU::getColorFromPalette(uint8_t paletteReg,
                                        uint8_t colorIndex) const {
  // Each color is represented by 2 bits in the palette register
  uint8_t shift = colorIndex * 2;
  uint8_t shade = (paletteReg >> shift) & 0x03;
  return shade;  // Return the shade (0-3)
}

uint8_t GameBoyPPU::readTilePixel(uint16_t tileDataAddr, int tileX,
                                  int tileY) const {
  // Each tile is 16 bytes: 2 bytes per row (8 pixels)
  uint16_t rowAddr = tileDataAddr + (tileY * 2);
  uint8_t byte1 = readVRAM(rowAddr);
  uint8_t byte2 = readVRAM(rowAddr + 1);

  // Pixels are stored in bits: bit 7 is leftmost pixel
  int bitIndex = 7 - tileX;
  uint8_t colorIndex =
      ((byte2 >> bitIndex) & 0x01) << 1 | ((byte1 >> bitIndex) & 0x01);

  return colorIndex;  // Return the color index (0-3)
}
