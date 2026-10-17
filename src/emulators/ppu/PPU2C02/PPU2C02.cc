#include "emulators/ppu/PPU2C02/PPU2C02.hh"

PPU2C02::PPU2C02(NesMirroring mirroring) : mirroring_(mirroring) { reset(); }

PPU2C02::~PPU2C02() {
  // Destructor implementation (if needed)
}

PPU2C02::reset() {
  // Reset the PPU state
  chr_rom_.fill(0);
  palette_.fill(0);
  vram_.fill(0);
  oam_data_.fill(0);
}

void PPU2C02::writePPUAddr(uint8_t data) { vram_addr_.update(data); }

void PPU2C02::writePPUCtrl(uint8_t data) { control_.update(data); }

void PPU2C02::incrementVramAddr() {
  uint8_t increment = control_.vram_addr_increment();
  vram_addr_.increment(increment);
}

uint8_t PPU2C02::readPPUData() {
  uint16_t addr = vram_addr_.get();
  incrementVramAddr();

  if (addr < 0x2000) {
    uint8_t result = internal_buffer_;
    internal_buffer_ = chr_rom_[addr];
    return result;
  } else if (addr >= 0x2000 && addr < 0x3F00) {
    uint8_t result = internal_buffer_;
    internal_buffer_ = vram_[mirrorVramAddr(addr)];
    return result;
  } else if (addr >= 0x3F00 && addr < 0x4000) {
    // Read from palette RAM
    uint16_t mirrored_addr = addr & 0x001F;  // Mirror down to 0x3F00-0x3F1F
    // Implement reading from palette RAM here
  } else {
    // Invalid address range, handle as needed
  }
  return 0xFF;  // Placeholder return value
}

uint16_t PPU2C02::mirrorVramAddr(uint16_t addr) {
  uint16_t mirrored_addr = addr & 0x2FFF;        // Mirror down to 0x2000-0x2FFF
  uint16_t vram_index = mirrored_addr - 0x2000;  // Index into vram_ array
  uint16_t name_table_index = vram_index / 0x400;  // Determine which name table
  switch (mirroring) {
    case Mirroring::Vertical:
      if (name_table == 2 || name_table == 3) {
        return vram_index - 0x800;
      }
      break;

    case Mirroring::Horizontal:
      switch (name_table) {
        case 1:
          return vram_index - 0x400;
        case 2:
          return vram_index - 0x400;
        case 3:
          return vram_index - 0x800;
        default:
          break;
      }
      break;

    default:
      break;
  }
  return vram_index;
}
