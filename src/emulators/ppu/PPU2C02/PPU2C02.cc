#include "emulators/ppu/PPU2C02/PPU2C02.hh"

namespace {

/// Reverse the bits of a byte (horizontal sprite flip).
uint8_t Flip(uint8_t b) {
  b = static_cast<uint8_t>((b & 0xF0) >> 4 | (b & 0x0F) << 4);
  b = static_cast<uint8_t>((b & 0xCC) >> 2 | (b & 0x33) << 2);
  b = static_cast<uint8_t>((b & 0xAA) >> 1 | (b & 0x55) << 1);
  return b;
}

}  // namespace

PPU2C02::PPU2C02(NesCartridge* cartridge) : cart_(cartridge) { reset(); }

void PPU2C02::reset() {
  s_ = PPU2C02State{};
  s_.status = 0;
  frame_.fill(0);
}

uint8_t PPU2C02::paletteIndex(uint16_t addr) {
  uint8_t idx = addr & 0x1F;
  if ((idx & 0x13) == 0x10)
    idx &= static_cast<uint8_t>(~0x10);  // $3F10 -> $3F00
  return idx;
}

uint8_t PPU2C02::read(uint16_t addr) {
  addr &= 0x3FFF;
  if (addr < 0x2000) return cart_->ppuRead(addr);
  if (addr < 0x3F00) {
    return s_.vram[MirrorNametableAddress(addr, cart_->getMirroring())];
  }
  return s_.palette[paletteIndex(addr)];
}

void PPU2C02::write(uint16_t addr, uint8_t value) {
  addr &= 0x3FFF;
  if (addr < 0x2000) {
    cart_->ppuWrite(addr, value);
  } else if (addr < 0x3F00) {
    s_.vram[MirrorNametableAddress(addr, cart_->getMirroring())] = value;
  } else {
    s_.palette[paletteIndex(addr)] = value & 0x3F;
  }
}

/// Pattern table fetch made by the rendering pipeline. Tracks A12 so that
/// mapper scanline counters (MMC3) see the filtered rising edges.
uint8_t PPU2C02::fetchPattern(uint16_t addr) {
  trackA12(addr);
  return cart_->ppuRead(addr);
}

/// Follow the A12 address line. A rising edge that comes after A12 stayed low
/// for a while clocks the mapper (MMC3 scanline counter).
void PPU2C02::trackA12(uint16_t addr) {
  const bool a12 = (addr & 0x1000) != 0;
  if (a12 && !s_.a12 && s_.dot_counter - s_.a12_low_since >= 10) {
    cart_->onScanline();
  }
  if (!a12 && s_.a12) s_.a12_low_since = s_.dot_counter;
  s_.a12 = a12;
}

void PPU2C02::updateNmiLine() {
  const bool line = (s_.ctrl & kCtrlNmi) && (s_.status & kStatusVBlank);
  if (line && !s_.nmi_line) s_.nmi_pending = true;
  s_.nmi_line = line;
}

uint8_t PPU2C02::readRegister(uint16_t addr) {
  uint8_t result = s_.io_latch;
  switch (addr & 7) {
    case 2:  // PPUSTATUS
      result = static_cast<uint8_t>((s_.status & 0xE0) | (s_.io_latch & 0x1F));
      s_.status &= static_cast<uint8_t>(~kStatusVBlank);
      s_.w = false;
      updateNmiLine();
      break;
    case 4:  // OAMDATA
      result = s_.oam[s_.oam_addr];
      if ((s_.oam_addr & 3) == 2) result &= 0xE3;  // unused attribute bits
      break;
    case 7: {  // PPUDATA
      const uint16_t a = s_.v & 0x3FFF;
      if (a >= 0x3F00) {
        // Palette reads are immediate; the buffer gets the nametable below.
        result = static_cast<uint8_t>((s_.palette[paletteIndex(a)] & 0x3F) |
                                      (s_.io_latch & 0xC0));
        s_.read_buffer = read(static_cast<uint16_t>(a - 0x1000));
      } else {
        result = s_.read_buffer;
        s_.read_buffer = read(a);
      }
      s_.v = static_cast<uint16_t>(
          (s_.v + ((s_.ctrl & kCtrlIncrement) ? 32 : 1)) & 0x7FFF);
      if (!renderingEnabled()) trackA12(s_.v);
      break;
    }
    default:
      break;  // write-only registers return the latch
  }
  s_.io_latch = result;
  return result;
}

void PPU2C02::writeRegister(uint16_t addr, uint8_t value) {
  s_.io_latch = value;
  switch (addr & 7) {
    case 0:  // PPUCTRL
      s_.ctrl = value;
      s_.t = static_cast<uint16_t>((s_.t & ~0x0C00) | ((value & 3) << 10));
      updateNmiLine();
      break;
    case 1:  // PPUMASK
      s_.mask = value;
      break;
    case 3:  // OAMADDR
      s_.oam_addr = value;
      break;
    case 4:  // OAMDATA
      s_.oam[s_.oam_addr++] = value;
      break;
    case 5:  // PPUSCROLL
      if (!s_.w) {
        s_.t = static_cast<uint16_t>((s_.t & ~0x001F) | (value >> 3));
        s_.fine_x = value & 7;
      } else {
        s_.t = static_cast<uint16_t>((s_.t & ~0x73E0) | ((value & 7) << 12) |
                                     ((value >> 3) << 5));
      }
      s_.w = !s_.w;
      break;
    case 6:  // PPUADDR
      if (!s_.w) {
        s_.t = static_cast<uint16_t>((s_.t & 0x00FF) | ((value & 0x3F) << 8));
      } else {
        s_.t = static_cast<uint16_t>((s_.t & 0xFF00) | value);
        s_.v = s_.t;
        if (!renderingEnabled()) trackA12(s_.v);
      }
      s_.w = !s_.w;
      break;
    case 7:  // PPUDATA
      write(s_.v, value);
      s_.v = static_cast<uint16_t>(
          (s_.v + ((s_.ctrl & kCtrlIncrement) ? 32 : 1)) & 0x7FFF);
      if (!renderingEnabled()) trackA12(s_.v);
      break;
    default:
      break;  // $2002 is read-only
  }
}

void PPU2C02::incrementX() {
  if ((s_.v & 0x001F) == 31) {
    s_.v = static_cast<uint16_t>((s_.v & ~0x001F) ^ 0x0400);
  } else {
    ++s_.v;
  }
}

void PPU2C02::incrementY() {
  if ((s_.v & 0x7000) != 0x7000) {
    s_.v = static_cast<uint16_t>(s_.v + 0x1000);
    return;
  }
  s_.v &= ~0x7000;
  uint16_t y = (s_.v & 0x03E0) >> 5;
  if (y == 29) {
    y = 0;
    s_.v ^= 0x0800;
  } else if (y == 31) {
    y = 0;
  } else {
    ++y;
  }
  s_.v = static_cast<uint16_t>((s_.v & ~0x03E0) | (y << 5));
}

void PPU2C02::copyX() {
  s_.v = static_cast<uint16_t>((s_.v & ~0x041F) | (s_.t & 0x041F));
}

void PPU2C02::copyY() {
  s_.v = static_cast<uint16_t>((s_.v & ~0x7BE0) | (s_.t & 0x7BE0));
}

void PPU2C02::loadBgShifters() {
  s_.bg_lo = static_cast<uint16_t>((s_.bg_lo & 0xFF00) | s_.pat_lo);
  s_.bg_hi = static_cast<uint16_t>((s_.bg_hi & 0xFF00) | s_.pat_hi);
  s_.at_lo = static_cast<uint16_t>((s_.at_lo & 0xFF00) |
                                   ((s_.at_byte & 1) ? 0xFF : 0x00));
  s_.at_hi = static_cast<uint16_t>((s_.at_hi & 0xFF00) |
                                   ((s_.at_byte & 2) ? 0xFF : 0x00));
}

/// Find the sprites that appear on the next scanline.
void PPU2C02::evaluateSprites() {
  const int height = (s_.ctrl & kCtrlSprite16) ? 16 : 8;
  s_.sprite_count = 0;
  s_.sprite0_in_line = false;
  for (PpuSprite& sp : s_.sprites) sp = PpuSprite{};

  for (int n = 0; n < 64; ++n) {
    const int diff = s_.scanline - s_.oam[n * 4];
    if (diff < 0 || diff >= height) continue;
    if (s_.sprite_count >= 8) {
      s_.status |= kStatusOverflow;
      break;
    }
    PpuSprite& sp = s_.sprites[s_.sprite_count++];
    sp.y = s_.oam[n * 4];
    sp.tile = s_.oam[n * 4 + 1];
    sp.attr = s_.oam[n * 4 + 2];
    sp.x = s_.oam[n * 4 + 3];
    if (n == 0) s_.sprite0_in_line = true;
  }
}

/// Fetch one pattern byte of sprite slot @p slot (dummy tile $FF if unused).
void PPU2C02::fetchSprite(int slot, bool high) {
  PpuSprite& sp = s_.sprites[static_cast<size_t>(slot)];
  const bool used = slot < s_.sprite_count;
  const bool tall = (s_.ctrl & kCtrlSprite16) != 0;
  const bool flip_v = used && (sp.attr & 0x80);

  int row = used ? s_.scanline - sp.y : 0;
  uint16_t addr = 0;
  if (!tall) {
    if (flip_v) row = 7 - row;
    addr = static_cast<uint16_t>(((s_.ctrl & kCtrlSpritePattern) ? 0x1000 : 0) |
                                 (sp.tile << 4) | (row & 7));
  } else {
    if (flip_v) row = 15 - row;
    const uint16_t table = (sp.tile & 1) ? 0x1000 : 0x0000;
    const uint16_t tile =
        static_cast<uint16_t>((sp.tile & 0xFE) + (row >= 8 ? 1 : 0));
    addr = static_cast<uint16_t>(table | (tile << 4) | (row & 7));
  }
  if (high) addr = static_cast<uint16_t>(addr + 8);

  uint8_t data = fetchPattern(addr);
  if (!used) data = 0;
  if (used && (sp.attr & 0x40)) data = Flip(data);
  (high ? sp.hi : sp.lo) = data;
}

void PPU2C02::renderPixel() {
  const int x = s_.dot - 1;
  const int y = s_.scanline;
  const bool show_bg =
      (s_.mask & kMaskBg) && (x >= 8 || (s_.mask & kMaskBgLeft));
  const bool show_sp =
      (s_.mask & kMaskSprites) && (x >= 8 || (s_.mask & kMaskSpritesLeft));

  uint8_t bg_pixel = 0;
  uint8_t bg_pal = 0;
  if (show_bg) {
    const uint16_t bit = static_cast<uint16_t>(0x8000 >> s_.fine_x);
    bg_pixel = static_cast<uint8_t>(((s_.bg_hi & bit) ? 2 : 0) |
                                    ((s_.bg_lo & bit) ? 1 : 0));
    bg_pal = static_cast<uint8_t>(((s_.at_hi & bit) ? 2 : 0) |
                                  ((s_.at_lo & bit) ? 1 : 0));
  }

  uint8_t sp_pixel = 0;
  uint8_t sp_pal = 0;
  bool sp_front = false;
  bool sp_is0 = false;
  if (show_sp) {
    for (int i = 0; i < s_.sprite_count; ++i) {
      const PpuSprite& sp = s_.sprites[static_cast<size_t>(i)];
      if (sp.x != 0) continue;
      const uint8_t px = static_cast<uint8_t>(((sp.hi & 0x80) ? 2 : 0) |
                                              ((sp.lo & 0x80) ? 1 : 0));
      if (px == 0) continue;
      sp_pixel = px;
      sp_pal = static_cast<uint8_t>((sp.attr & 3) + 4);
      sp_front = (sp.attr & 0x20) == 0;
      sp_is0 = (i == 0 && s_.sprite0_in_line);
      break;
    }
  }

  if (sp_is0 && bg_pixel != 0 && sp_pixel != 0 && x != 255 &&
      (s_.mask & kMaskBg) && (s_.mask & kMaskSprites)) {
    s_.status |= kStatusSprite0;
  }

  uint8_t color_idx = 0;
  if (bg_pixel == 0 && sp_pixel == 0) {
    color_idx = 0;
  } else if (bg_pixel == 0) {
    color_idx = static_cast<uint8_t>((sp_pal << 2) | sp_pixel);
  } else if (sp_pixel == 0) {
    color_idx = static_cast<uint8_t>((bg_pal << 2) | bg_pixel);
  } else {
    color_idx = sp_front ? static_cast<uint8_t>((sp_pal << 2) | sp_pixel)
                         : static_cast<uint8_t>((bg_pal << 2) | bg_pixel);
  }

  uint8_t color = s_.palette[paletteIndex(color_idx)];
  if (s_.mask & kMaskGrey) color &= 0x30;
  frame_[static_cast<size_t>(y * kWidth + x)] =
      static_cast<uint16_t>(color | ((s_.mask >> 5) << 6));
}

void PPU2C02::step() {
  ++s_.dot_counter;
  const bool rendering = renderingEnabled();
  const bool visible_line = s_.scanline >= 0 && s_.scanline < 240;
  const bool render_line = visible_line || s_.scanline == -1;

  if (render_line) {
    if (s_.scanline == -1 && s_.dot == 1) {
      s_.status &= static_cast<uint8_t>(
          ~(kStatusVBlank | kStatusSprite0 | kStatusOverflow));
      updateNmiLine();
    }

    if (rendering) {
      const int d = s_.dot;

      // Background / sprite shifters advance every dot of the fetch windows.
      if ((d >= 2 && d <= 257) || (d >= 321 && d <= 337)) {
        s_.bg_lo = static_cast<uint16_t>(s_.bg_lo << 1);
        s_.bg_hi = static_cast<uint16_t>(s_.bg_hi << 1);
        s_.at_lo = static_cast<uint16_t>(s_.at_lo << 1);
        s_.at_hi = static_cast<uint16_t>(s_.at_hi << 1);
      }

      if ((d >= 1 && d <= 256) || (d >= 321 && d <= 337)) {
        switch ((d - 1) & 7) {
          case 0:
            if (d > 1) loadBgShifters();
            s_.nt_byte = read(static_cast<uint16_t>(0x2000 | (s_.v & 0x0FFF)));
            break;
          case 2: {
            uint8_t at = read(static_cast<uint16_t>(0x23C0 | (s_.v & 0x0C00) |
                                                    ((s_.v >> 4) & 0x38) |
                                                    ((s_.v >> 2) & 0x07)));
            if (s_.v & 0x40) at >>= 4;
            if (s_.v & 0x02) at >>= 2;
            s_.at_byte = at & 3;
            break;
          }
          case 4:
            s_.pat_lo = fetchPattern(static_cast<uint16_t>(
                ((s_.ctrl & kCtrlBgPattern) ? 0x1000 : 0) | (s_.nt_byte << 4) |
                ((s_.v >> 12) & 7)));
            break;
          case 6:
            s_.pat_hi = fetchPattern(static_cast<uint16_t>(
                ((s_.ctrl & kCtrlBgPattern) ? 0x1000 : 0) | (s_.nt_byte << 4) |
                ((s_.v >> 12) & 7) | 8));
            break;
          case 7:
            incrementX();
            break;
        }
      }
      if (d == 256) incrementY();
      if (d == 257) {
        loadBgShifters();
        copyX();
        if (visible_line) {
          evaluateSprites();
        } else {
          s_.sprite_count = 0;
          s_.sprite0_in_line = false;
        }
      }
      if (d == 339) {
        s_.nt_byte = read(static_cast<uint16_t>(0x2000 | (s_.v & 0x0FFF)));
      }
      if (s_.scanline == -1 && d >= 280 && d <= 304) copyY();

      // Sprite pattern fetches for the next scanline.
      if (d >= 257 && d <= 320) {
        const int slot = (d - 257) >> 3;
        const int phase = (d - 257) & 7;
        if (phase == 4) fetchSprite(slot, false);
        if (phase == 6) fetchSprite(slot, true);
      }
    }

    if (visible_line && s_.dot >= 1 && s_.dot <= 256) {
      if (rendering) {
        renderPixel();
      } else {
        // Backdrop colour; if v points into palette RAM it is shown instead.
        const uint8_t idx =
            ((s_.v & 0x3F00) == 0x3F00) ? paletteIndex(s_.v) : 0;
        uint8_t color = s_.palette[idx];
        if (s_.mask & kMaskGrey) color &= 0x30;
        frame_[static_cast<size_t>(s_.scanline * kWidth + s_.dot - 1)] =
            static_cast<uint16_t>(color | ((s_.mask >> 5) << 6));
      }
    }
  }

  // After the pixel is drawn: sprite x counters count down, then the
  // pattern shifters of the sprites that reached their column advance.
  if (rendering && visible_line && s_.dot >= 1 && s_.dot <= 256) {
    for (int i = 0; i < s_.sprite_count; ++i) {
      PpuSprite& sp = s_.sprites[static_cast<size_t>(i)];
      if (sp.x > 0) {
        --sp.x;
      } else {
        sp.lo = static_cast<uint8_t>(sp.lo << 1);
        sp.hi = static_cast<uint8_t>(sp.hi << 1);
      }
    }
  }

  if (s_.scanline == 241 && s_.dot == 1) {
    s_.status |= kStatusVBlank;
    s_.frame_ready = true;
    updateNmiLine();
  }

  // Advance the beam.
  ++s_.dot;
  if (s_.scanline == -1 && s_.dot == 340 && s_.odd_frame && rendering) {
    s_.dot = 341;  // the odd-frame pre-render line is one dot shorter
  }
  if (s_.dot > 340) {
    s_.dot = 0;
    ++s_.scanline;
    if (s_.scanline > 260) {
      s_.scanline = -1;
      s_.odd_frame = !s_.odd_frame;
    }
  }
}
