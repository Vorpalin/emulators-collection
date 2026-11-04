#include "emulators/bus/nes_bus/NesBus.hh"

#include <fstream>
#include <iterator>
#include <vector>

void to_json(nlohmann::json& j, const NesBusState& s) {
  j = nlohmann::json{{"cpu", s.cpu},
                     {"ram", s.ram},
                     {"ppu", s.ppu},
                     {"apu", s.apu},
                     {"pads", s.pads},
                     {"cartridge", s.cartridge},
                     {"cpu_cycles", s.cpu_cycles},
                     {"dma_stall", s.dma_stall},
                     {"open_bus", s.open_bus}};
}

void from_json(const nlohmann::json& j, NesBusState& s) {
  j.at("cpu").get_to(s.cpu);
  j.at("ram").get_to(s.ram);
  j.at("ppu").get_to(s.ppu);
  j.at("apu").get_to(s.apu);
  j.at("pads").get_to(s.pads);
  s.cartridge = j.at("cartridge");
  j.at("cpu_cycles").get_to(s.cpu_cycles);
  j.at("dma_stall").get_to(s.dma_stall);
  j.at("open_bus").get_to(s.open_bus);
}

NesBus::NesBus() : ppu_(&cartridge_), cpu_(this) {
  cpu_.setDecimalModeEnabled(false);  // the 2A03 has no BCD arithmetic
  apu_.setMemoryReader([this](uint16_t addr) {
    dma_stall_ += 4;  // a DMC fetch steals CPU cycles
    return read(addr);
  });
}

void NesBus::reset() {
  ram_.fill(0);
  cartridge_.reset();
  ppu_.reset();
  apu_.reset();
  for (NesController& p : pads_) p.reset();
  cpu_cycles_ = 0;
  dma_stall_ = 0;
  open_bus_ = 0;
  cpu_.reset();
  advance(7);  // the reset sequence takes 7 cycles
}

void NesBus::loadROM(std::string& filename) {
  std::ifstream in(filename, std::ios::binary);
  const std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)),
                                  std::istreambuf_iterator<char>());
  loadROM(data.data(), data.size());
}

bool NesBus::loadROM(const uint8_t* data, std::size_t size) {
  if (!cartridge_.load(data, size)) return false;
  reset();
  return true;
}

uint8_t NesBus::read(uint16_t address) {
  uint8_t value = open_bus_;
  if (address < 0x2000) {
    value = ram_[address & 0x7FF];
  } else if (address < 0x4000) {
    value = ppu_.readRegister(address);
  } else if (address < 0x4020) {
    switch (address) {
      case 0x4015:
        value = apu_.read(address);
        break;
      case 0x4016:
      case 0x4017:
        // Only bit 0 is driven by the pad; the rest is open bus.
        value = static_cast<uint8_t>((open_bus_ & 0xE0) |
                                     pads_[address & 1].read());
        break;
      default:
        break;  // write-only / unmapped: open bus
    }
  } else {
    value = cartridge_.cpuRead(address);
  }
  open_bus_ = value;
  return value;
}

void NesBus::write(uint16_t address, uint8_t value) {
  open_bus_ = value;
  if (address < 0x2000) {
    ram_[address & 0x7FF] = value;
  } else if (address < 0x4000) {
    ppu_.writeRegister(address, value);
  } else if (address < 0x4020) {
    if (address == 0x4014) {
      // OAM DMA: copy a 256-byte page, stalling the CPU 513/514 cycles.
      const uint16_t base = static_cast<uint16_t>(value << 8);
      for (uint16_t i = 0; i < 256; ++i) {
        ppu_.oamDmaWrite(read(static_cast<uint16_t>(base + i)));
      }
      dma_stall_ += 513 + static_cast<uint32_t>(cpu_cycles_ & 1);
    } else if (address == 0x4016) {
      pads_[0].write(value);
      pads_[1].write(value);
    } else {
      apu_.write(address, value);  // also handles $4017 (frame counter)
    }
  } else {
    cartridge_.cpuWrite(address, value);
  }
}

void NesBus::advance(uint32_t cpuCycles) {
  for (uint32_t i = 0; i < cpuCycles * 3; ++i) ppu_.step();
  apu_.step(cpuCycles);
  cpu_cycles_ += cpuCycles;
}

uint32_t NesBus::step() {
  const CPU65State st = cpu_.getState();
  uint32_t cycles = 0;
  if (ppu_.pollNmi()) {
    cycles = cpu_.nmi();
    irq_poll_delayed_ = false;
  } else {
    // CLI/SEI/PLP change I only after the interrupt lines were polled, so the
    // instruction that follows still sees the old mask.
    const bool masked = irq_poll_delayed_ ? prev_i_ : st.I;
    if ((cartridge_.irqPending() || apu_.irqPending()) && !masked) {
      cycles = cpu_.irq(true);
      irq_poll_delayed_ = false;
    }
  }
  if (cycles == 0) {
    const uint8_t op = peek(st.PC);
    irq_poll_delayed_ = op == 0x58 || op == 0x78 || op == 0x28;  // CLI SEI PLP
    prev_i_ = st.I;
    cycles = cpu_.execute();
  }
  if (cycles == 0) cycles = 1;  // jammed CPU: let time pass anyway
  cycles += dma_stall_;
  dma_stall_ = 0;
  advance(cycles);
  return cycles;
}

/// Side-effect free read used to look at the next opcode.
uint8_t NesBus::peek(uint16_t address) {
  if (address < 0x2000) return ram_[address & 0x7FF];
  if (address >= 0x4020) return cartridge_.cpuRead(address);
  return 0;
}

void NesBus::setState(const NesBusState& s) {
  cpu_.setState(s.cpu);
  ram_ = s.ram;
  ppu_.setState(s.ppu);
  apu_.setState(s.apu);
  pads_ = s.pads;
  cartridge_.loadState(s.cartridge);
  cpu_cycles_ = s.cpu_cycles;
  dma_stall_ = s.dma_stall;
  open_bus_ = s.open_bus;
}

NesBusState NesBus::getState() const {
  return NesBusState{cpu_.getState(), ram_,       ppu_.getState(),
                     apu_.getState(), pads_,      cartridge_.saveState(),
                     cpu_cycles_,     dma_stall_, open_bus_};
}
