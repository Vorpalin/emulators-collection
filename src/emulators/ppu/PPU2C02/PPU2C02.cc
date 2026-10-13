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
