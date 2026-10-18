#include "emulators/bus/nes_bus/NesBus.hh"

void to_json(nlohmann::json& j, const NesBusState& state) {
  j = nlohmann::json{{"cpu", state.cpu}, {"ram", state.ram}};
}

void from_json(const nlohmann::json& j, NesBusState& state) {
  j.at("cpu").get_to(state.cpu);
  j.at("ram").get_to(state.ram);
}

NesBus::NesBus() : cpu(this) {}

NesBus::~NesBus() {}

void NesBus::reset() { cpu.reset(); }

void NesBus::setState(const NesBusState& state) { cpu.setState(state.cpu); }

NesBusState NesBus::getState() const { return NesBusState{cpu.getState()}; }

void NesBus::loadROM(std::string& filename) {
  // FIXME : Implement NES ROM loading logic here.
  (void)filename;  // Suppress unused variable warning for now.
}

void NesBus::tick() { cpu.tick(); }

void NesBus::write(uint16_t address, uint8_t value) {
  if (address < 0x2000) {
    for (int i = 0; i < 4; ++i) {
      ram[(address + i * 0x800) % 0x2000] =
          value;  // NES RAM is mirrored every 2KB
    }
  } else if (address < 0x4000) {
    // Registres PPU ($2000-$2007), miroir tous les 8 octets jusqu'à $3FFF
    switch (address & 0x2007) {
      case 0x2000:
        ppu.writePPUCtrl(value);
        break;
      case 0x2001: /* ppu.writeMask(value); */
        break;
      case 0x2002: /* PPUSTATUS : read only */
        break;
      case 0x2003: /* ppu.writeOAMAddr(value); */
        break;
      case 0x2004: /* ppu.writeOAMData(value); */
        break;
      case 0x2005: /* ppu.writeScroll(value); */
        break;
      case 0x2006:
        ppu.writePPUAddr(value);
        break;
      case 0x2007: /* ppu.writePPUData(value); */
        break;
    }
  } else if (address == 0x4014) {
    uint8_t buffer[256];
    uint16_t hi = static_cast<uint16_t>(value) << 8;
    for (int i = 0; i < 256; ++i) {
      buffer[i] = read(hi + i);
    }
    ppu.write_oam_dma(buffer);
  } else {
    cpu.write(address, value);
  }
}

uint8_t NesBus::read(uint16_t address) {
  if (address < 0x2000) {
    return ram[address % 0x2000];  // NES RAM is mirrored every 2KB
  } else if (address < 0x4000) {
    switch (address & 0x2007) {
      case 0x2002:
        return ppu.readPPUStatus();
      case 0x2004:
        return ppu.readOAMData();
      case 0x2007:
        return ppu.readPPUData();
      default:
        return 0;
    }
  } else if (address == 0x4014) {
    return 0;
  } else {
    return cpu.read(address);
  }
}
