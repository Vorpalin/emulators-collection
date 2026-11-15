#include "emulators/ppu/game_boy_ppu/GameBoyPPU.hh"

#include <algorithm>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

constexpr int CYCLES_OAM_SCAN = 80;
constexpr int CYCLES_DRAWING = 172;
constexpr int CYCLES_LINE = 456;
constexpr int LINES_VBLANK_END = 153;

void to_json(nlohmann::json& j, const PPURegisters& regs) {
  j = {
      {"lcdc", regs.lcdc}, {"stat", regs.stat}, {"scy", regs.scy},
      {"scx", regs.scx},   {"ly", regs.ly},     {"lyc", regs.lyc},
      {"dma", regs.dma},   {"bgp", regs.bgp},   {"obp0", regs.obp0},
      {"obp1", regs.obp1}, {"wy", regs.wy},     {"wx", regs.wx},
  };
}

void from_json(const nlohmann::json& j, PPURegisters& regs) {
  j.at("lcdc").get_to(regs.lcdc);
  j.at("stat").get_to(regs.stat);
  j.at("scy").get_to(regs.scy);
  j.at("scx").get_to(regs.scx);
  j.at("ly").get_to(regs.ly);
  j.at("lyc").get_to(regs.lyc);
  j.at("dma").get_to(regs.dma);
  j.at("bgp").get_to(regs.bgp);
  j.at("obp0").get_to(regs.obp0);
  j.at("obp1").get_to(regs.obp1);
  j.at("wy").get_to(regs.wy);
  j.at("wx").get_to(regs.wx);
}

void to_json(nlohmann::json& j, const GameBoyPPUState& state) {
  j = {
      {"regs", state.regs},
      {"vram", state.vram},
      {"oam", state.oam},
      {"framebuffer", state.framebuffer},
      {"dotCounter", state.dotCounter},
      {"windowLine", state.windowLine},
      {"cgbMode", state.cgbMode},
      {"vram1", state.vram1},
      {"vramBank", state.vramBank},
      {"bcps", state.bcps},
      {"ocps", state.ocps},
      {"opri", state.opri},
      {"bgPalette", state.bgPalette},
      {"objPalette", state.objPalette},
      {"colorFramebuffer", state.colorFramebuffer},
  };
}

void from_json(const nlohmann::json& j, GameBoyPPUState& state) {
  j.at("regs").get_to(state.regs);
  j.at("vram").get_to(state.vram);
  j.at("oam").get_to(state.oam);
  j.at("framebuffer").get_to(state.framebuffer);
  j.at("dotCounter").get_to(state.dotCounter);

  // Game Boy Color fields: optional so that DMG-era save states still load.
  state.windowLine = j.value("windowLine", 0);
  state.cgbMode = j.value("cgbMode", false);
  if (j.contains("vram1")) j.at("vram1").get_to(state.vram1);
  state.vramBank = j.value("vramBank", static_cast<uint8_t>(0));
  state.bcps = j.value("bcps", static_cast<uint8_t>(0));
  state.ocps = j.value("ocps", static_cast<uint8_t>(0));
  state.opri = j.value("opri", static_cast<uint8_t>(0));
  if (j.contains("bgPalette")) j.at("bgPalette").get_to(state.bgPalette);
  if (j.contains("objPalette")) j.at("objPalette").get_to(state.objPalette);
  if (j.contains("colorFramebuffer"))
    j.at("colorFramebuffer").get_to(state.colorFramebuffer);
}

void GameBoyPPU::setState(const GameBoyPPUState& state) {
  regs_ = state.regs;
  vram = state.vram;
  oam = state.oam;
  framebuffer_ = state.framebuffer;
  dotCounter_ = state.dotCounter;
  windowLineCounter_ = state.windowLine;

  cgbMode_ = state.cgbMode;
  vram1_ = state.vram1;
  vramBank_ = state.vramBank;
  bcps_ = state.bcps;
  ocps_ = state.ocps;
  opri_ = state.opri;
  bgPalRam_ = state.bgPalette;
  objPalRam_ = state.objPalette;
  colorFb_ = state.colorFramebuffer;
}

GameBoyPPUState GameBoyPPU::getState() const {
  GameBoyPPUState state;
  state.regs = regs_;
  state.vram = vram;
  state.oam = oam;
  state.framebuffer = framebuffer_;
  state.dotCounter = dotCounter_;
  state.windowLine = windowLineCounter_;

  state.cgbMode = cgbMode_;
  state.vram1 = vram1_;
  state.vramBank = vramBank_;
  state.bcps = bcps_;
  state.ocps = ocps_;
  state.opri = opri_;
  state.bgPalette = bgPalRam_;
  state.objPalette = objPalRam_;
  state.colorFramebuffer = colorFb_;
  return state;
}

GameBoyPPU::GameBoyPPU() { reset(); }

void GameBoyPPU::reset() {
  regs_ = PPURegisters{};
  vram.fill(0);
  oam.fill(0);
  framebuffer_.fill(0);
  dotCounter_ = 0;
  windowLineCounter_ = 0;

  vram1_.fill(0);
  vramBank_ = 0;
  bcps_ = 0;
  ocps_ = 0;
  opri_ = 0;
  // Colour RAM powers up as white so an unprogrammed palette is not black.
  for (std::size_t i = 0; i < bgPalRam_.size(); i += 2) {
    bgPalRam_[i] = 0xFF;
    bgPalRam_[i + 1] = 0x7F;
    objPalRam_[i] = 0xFF;
    objPalRam_[i + 1] = 0x7F;
  }
  colorFb_.fill(0x7FFF);
  lineBgIndex_.fill(0);
  lineBgPriority_.fill(false);
}

void GameBoyPPU::tick(uint8_t cycles) {
  if (!regs_.lcdEnabled()) {
    // If LCD is disabled, reset the PPU state
    regs_.setMode(HBlank);
    regs_.ly = 0;
    dotCounter_ = 0;
    windowLineCounter_ = 0;
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
        if (hblankCallback_) hblankCallback_();
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
          windowLineCounter_ = 0;
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
  if (line < SCREEN_HEIGHT && cgbMode_) {
    // CGB: LCDC bit 0 no longer blanks the BG, it is the master priority bit.
    renderBgWindowLineCgb(line);
    if (regs_.spritesEnabled()) renderSpritesLineCgb(line);
    return;
  }
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
    uint8_t tileIndex = readVRAM(tileMapAddr - 0x8000);

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

  int windowLine = windowLineCounter_++;
  int tileRow = windowLine / 8;
  int pixelRowInTile = windowLine % 8;

  for (int x = std::max(0, windowX); x < SCREEN_WIDTH; x++) {
    int winPixelX = x - windowX;
    int tileCol = winPixelX / 8;
    int pixelColInTile = winPixelX % 8;

    uint16_t tileMapAddr = tileMapBase + (tileRow * 32) + tileCol;
    uint8_t tileIndex = readVRAM(tileMapAddr - 0x8000);

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
    (vramBank_ ? vram1_ : vram)[addr] = value;
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
    case 0xFF4F:  // VBK
      if (cgbMode_) vramBank_ = value & 0x01;
      break;
    case 0xFF68:  // BCPS
      if (cgbMode_) bcps_ = value & 0xBF;
      break;
    case 0xFF69:  // BCPD
      if (cgbMode_) {
        bgPalRam_[bcps_ & 0x3F] = value;
        if (bcps_ & 0x80) bcps_ = (bcps_ & 0x80) | ((bcps_ + 1) & 0x3F);
      }
      break;
    case 0xFF6A:  // OCPS
      if (cgbMode_) ocps_ = value & 0xBF;
      break;
    case 0xFF6B:  // OCPD
      if (cgbMode_) {
        objPalRam_[ocps_ & 0x3F] = value;
        if (ocps_ & 0x80) ocps_ = (ocps_ & 0x80) | ((ocps_ + 1) & 0x3F);
      }
      break;
    case 0xFF6C:  // OPRI
      if (cgbMode_) opri_ = value & 0x01;
      break;
  }
}

void GameBoyPPU::write(uint16_t address, uint8_t value) {
  if (address >= 0x8000 && address <= 0x9FFF) {
    writeVRAM(address - 0x8000, value);
  } else if (address >= 0xFE00 && address <= 0xFE9F) {
    writeOAM(address - 0xFE00, value);
  } else if ((address >= 0xFF40 && address <= 0xFF4B) || address == 0xFF4F ||
             (address >= 0xFF68 && address <= 0xFF6C)) {
    writeRegister(address, value);
  }
}

uint8_t GameBoyPPU::readVRAM(uint16_t addr) const {
  if (addr < 0x2000) {
    return vramAt(vramBank_, addr);
  }
  return 0xFF;  // Invalid read
}

uint8_t GameBoyPPU::readOAM(uint16_t addr) const {
  if (addr < 0xA0) {
    return oam[addr];
  }
  return 0xFF;  // Invalid read
}

uint8_t GameBoyPPU::read(uint16_t address) {
  if (address >= 0x8000 && address <= 0x9FFF) {
    return readVRAM(address - 0x8000);
  } else if (address >= 0xFE00 && address <= 0xFE9F) {
    return readOAM(address - 0xFE00);
  } else if ((address >= 0xFF40 && address <= 0xFF4B) || address == 0xFF4F ||
             (address >= 0xFF68 && address <= 0xFF6C)) {
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
    case 0xFF4F:
      return cgbMode_ ? static_cast<uint8_t>(0xFE | vramBank_) : 0xFF;
    case 0xFF68:
      return cgbMode_ ? static_cast<uint8_t>(bcps_ | 0x40) : 0xFF;
    case 0xFF69:
      return cgbMode_ ? bgPalRam_[bcps_ & 0x3F] : 0xFF;
    case 0xFF6A:
      return cgbMode_ ? static_cast<uint8_t>(ocps_ | 0x40) : 0xFF;
    case 0xFF6B:
      return cgbMode_ ? objPalRam_[ocps_ & 0x3F] : 0xFF;
    case 0xFF6C:
      return cgbMode_ ? static_cast<uint8_t>(0xFE | opri_) : 0xFF;
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

uint8_t GameBoyPPU::readTilePixel(uint16_t tileDataAddr, int tileX, int tileY,
                                  int bank) const {
  // Each tile is 16 bytes: 2 bytes per row (8 pixels)
  uint16_t rowAddr = tileDataAddr + (tileY * 2);
  uint8_t byte1 = vramAt(bank, static_cast<uint16_t>(rowAddr - 0x8000));
  uint8_t byte2 = vramAt(bank, static_cast<uint16_t>(rowAddr + 1 - 0x8000));

  // Pixels are stored in bits: bit 7 is leftmost pixel
  int bitIndex = 7 - tileX;
  uint8_t colorIndex =
      ((byte2 >> bitIndex) & 0x01) << 1 | ((byte1 >> bitIndex) & 0x01);

  return colorIndex;  // Return the color index (0-3)
}

// ---------------------------------------------------------------------------
// Game Boy Color rendering
// ---------------------------------------------------------------------------

uint16_t GameBoyPPU::paletteColor(const std::array<uint8_t, 64>& ram,
                                  int palette, int colorIndex) {
  const int offset = (palette & 7) * 8 + (colorIndex & 3) * 2;
  return static_cast<uint16_t>((ram[offset] | (ram[offset + 1] << 8)) & 0x7FFF);
}

void GameBoyPPU::renderBgWindowLineCgb(int line) {
  // --- Background ---
  const uint16_t bgMapBase = regs_.bgTileMapHigh() ? 0x1C00 : 0x1800;
  const uint8_t scrolledY = static_cast<uint8_t>(line + regs_.scy);

  for (int x = 0; x < SCREEN_WIDTH; x++) {
    const uint8_t scrolledX = static_cast<uint8_t>(x + regs_.scx);
    const uint16_t mapOffset =
        static_cast<uint16_t>(bgMapBase + (scrolledY / 8) * 32 + scrolledX / 8);

    const uint8_t tileIndex = vram[mapOffset];  // indices always in bank 0
    const uint8_t attr = vram1_[mapOffset];     // attributes always in bank 1

    int row = scrolledY % 8;
    int col = scrolledX % 8;
    if (attr & 0x40) row = 7 - row;  // vertical flip
    if (attr & 0x20) col = 7 - col;  // horizontal flip

    uint16_t tileAddr;
    if (regs_.bgWindowDataLow()) {
      tileAddr = static_cast<uint16_t>(0x8000 + tileIndex * 16);
    } else {
      tileAddr = static_cast<uint16_t>(0x9000 +
                                       static_cast<int8_t>(tileIndex) * 16);
    }
    const uint8_t colorIndex =
        readTilePixel(tileAddr, col, row, (attr >> 3) & 1);

    lineBgIndex_[x] = colorIndex;
    lineBgPriority_[x] = (attr & 0x80) != 0;
    colorFb_[line * SCREEN_WIDTH + x] =
        paletteColor(bgPalRam_, attr & 0x07, colorIndex);
    framebuffer_[line * SCREEN_WIDTH + x] = colorIndex;
  }

  // --- Window ---
  if (!regs_.windowEnabled() || line < regs_.wy) return;
  const int windowX = static_cast<int>(regs_.wx) - 7;
  if (windowX >= SCREEN_WIDTH) return;

  const uint16_t winMapBase = regs_.windowTileMapHigh() ? 0x1C00 : 0x1800;
  const int windowLine = windowLineCounter_++;

  for (int x = std::max(0, windowX); x < SCREEN_WIDTH; x++) {
    const int winX = x - windowX;
    const uint16_t mapOffset = static_cast<uint16_t>(
        winMapBase + (windowLine / 8) * 32 + winX / 8);

    const uint8_t tileIndex = vram[mapOffset];
    const uint8_t attr = vram1_[mapOffset];

    int row = windowLine % 8;
    int col = winX % 8;
    if (attr & 0x40) row = 7 - row;
    if (attr & 0x20) col = 7 - col;

    uint16_t tileAddr;
    if (regs_.bgWindowDataLow()) {
      tileAddr = static_cast<uint16_t>(0x8000 + tileIndex * 16);
    } else {
      tileAddr = static_cast<uint16_t>(0x9000 +
                                       static_cast<int8_t>(tileIndex) * 16);
    }
    const uint8_t colorIndex =
        readTilePixel(tileAddr, col, row, (attr >> 3) & 1);

    lineBgIndex_[x] = colorIndex;
    lineBgPriority_[x] = (attr & 0x80) != 0;
    colorFb_[line * SCREEN_WIDTH + x] =
        paletteColor(bgPalRam_, attr & 0x07, colorIndex);
    framebuffer_[line * SCREEN_WIDTH + x] = colorIndex;
  }
}

void GameBoyPPU::renderSpritesLineCgb(int line) {
  const int spriteHeight = regs_.spriteSize8x16() ? 16 : 8;

  struct Entry {
    Sprite sprite;
    int oamIndex;
  };
  std::vector<Entry> visible;
  visible.reserve(10);

  // Same 10-sprites-per-line limit as the DMG, in OAM order.
  for (int i = 0; i < 40 && visible.size() < 10; i++) {
    const uint8_t* base = &oam[i * 4];
    Sprite s{base[0], base[1], base[2], base[3]};
    const int top = static_cast<int>(s.y) - 16;
    if (line >= top && line < top + spriteHeight) visible.push_back({s, i});
  }

  // Highest priority first. In CGB mode priority is the OAM index; when OPRI
  // bit 0 is set the DMG rule applies (smaller X wins, then OAM index).
  if (opri_ & 0x01) {
    std::stable_sort(visible.begin(), visible.end(),
                     [](const Entry& a, const Entry& b) {
                       return a.sprite.x < b.sprite.x;
                     });
  }

  // A pixel belongs to the first (highest priority) sprite that is opaque
  // there, even if the BG then hides it: lower-priority sprites must not
  // show through.
  std::array<bool, SCREEN_WIDTH> claimed{};
  const bool masterPriority = regs_.bgWindowEnabled();  // LCDC bit 0

  for (const auto& e : visible) {
    const Sprite& s = e.sprite;
    const int top = static_cast<int>(s.y) - 16;
    const int left = static_cast<int>(s.x) - 8;

    int rowInSprite = line - top;
    if (s.flipY()) rowInSprite = spriteHeight - 1 - rowInSprite;

    uint8_t tileIndex = s.tileIndex;
    if (spriteHeight == 16) tileIndex &= 0xFE;
    const uint16_t tileAddr = static_cast<uint16_t>(0x8000 + tileIndex * 16);

    for (int col = 0; col < 8; col++) {
      const int screenX = left + col;
      if (screenX < 0 || screenX >= SCREEN_WIDTH) continue;
      if (claimed[screenX]) continue;

      const int colInSprite = s.flipX() ? (7 - col) : col;
      const uint8_t colorIndex =
          readTilePixel(tileAddr, colInSprite, rowInSprite, s.cgbVramBank());
      if (colorIndex == 0) continue;  // transparent

      claimed[screenX] = true;

      if (masterPriority && lineBgIndex_[screenX] != 0 &&
          (s.priorityBehindBG() || lineBgPriority_[screenX])) {
        continue;  // BG/window wins
      }

      colorFb_[line * SCREEN_WIDTH + screenX] =
          paletteColor(objPalRam_, s.cgbPalette(), colorIndex);
      framebuffer_[line * SCREEN_WIDTH + screenX] = colorIndex;
    }
  }
}
