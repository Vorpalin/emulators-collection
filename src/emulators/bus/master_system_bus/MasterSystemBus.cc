#include "emulators/bus/master_system_bus/MasterSystemBus.hh"

#include <fstream>
#include <iterator>

MasterSystemBus::MasterSystemBus(SegaVDP::Model model)
    : model_(model), vdp_(model), cpu_(this) {
  vdp_.onInterrupt([this](bool level) { cpu_.setIRQ(level, 0xFF); });
  vdp_.onFrameReady([this] { frameReady_ = true; });
  reset();
}

bool MasterSystemBus::loadROM(const uint8_t *data, std::size_t size) {
  if (!cartridge_.load(data, size)) return false;
  reset();
  return true;
}

void MasterSystemBus::loadROM(std::string &filename) {
  std::ifstream in(filename, std::ios::binary);
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),
                             std::istreambuf_iterator<char>());
  loadROM(bytes.data(), bytes.size());
}

void MasterSystemBus::reset() {
  ram_.fill(0);
  cartridge_.reset();
  vdp_.reset();
  psg_.reset();
  cpu_.reset();
  // State left by the BIOS: cartridge mapped, stack at the top of RAM.
  Z80State s = cpu_.getState();
  s.SP = 0xDFF0;
  cpu_.setState(s);
  memoryControl_ = 0xAB;
  ioControl_ = 0xFF;
  pad_.fill(0);
  start_ = pause_ = frameReady_ = false;
  cpuCycles_ = 0;
}

void MasterSystemBus::setPAL(bool pal) {
  vdp_.setPAL(pal);
  psg_.setPAL(pal);
}

uint32_t MasterSystemBus::step() {
  const uint32_t cycles = cpu_.execute();
  cpuCycles_ += cycles;
  vdp_.tick(cycles);
  psg_.step(cycles);
  return cycles;
}

// ---------------------------------------------------------------------------
// Memory
// ---------------------------------------------------------------------------

uint8_t MasterSystemBus::read(uint16_t address) {
  if (address < 0xC000) {
    return (memoryControl_ & 0x40) ? 0xFF : cartridge_.read(address);
  }
  return (memoryControl_ & 0x10) ? 0xFF : ram_[address & 0x1FFF];
}

void MasterSystemBus::write(uint16_t address, uint8_t value) {
  if (address < 0xC000) {
    if (!(memoryControl_ & 0x40)) cartridge_.write(address, value);
    return;
  }
  if (!(memoryControl_ & 0x10)) ram_[address & 0x1FFF] = value;
  if (address >= 0xFFFC) cartridge_.writeMapper(address, value);
}

// ---------------------------------------------------------------------------
// I/O ports
// ---------------------------------------------------------------------------

uint8_t MasterSystemBus::joypadA() const {
  // Bits 0-5: player 1 up, down, left, right, button 1, button 2.
  // Bits 6-7: player 2 up, down. All active low.
  uint8_t v = static_cast<uint8_t>(
      ~((pad_[0] & 0x3F) | ((pad_[1] & 0x03) << 6)));
  if (!(ioControl_ & 0x01)) {  // TR of port A configured as output.
    v = static_cast<uint8_t>((v & ~0x20) | (((ioControl_ >> 4) & 1) << 5));
  }
  return v;
}

uint8_t MasterSystemBus::joypadB() const {
  // Bits 0-3: player 2 left, right, button 1, button 2 (active low).
  // Bit 4: reset button (not wired, reads 1). Bits 6-7: TH lines.
  uint8_t v = static_cast<uint8_t>(~((pad_[1] >> 2) & 0x0F) & 0x0F);
  v |= 0xF0;
  if (!(ioControl_ & 0x04))  // TR of port B as output.
    v = static_cast<uint8_t>((v & ~0x08) | (((ioControl_ >> 6) & 1) << 3));
  if (!(ioControl_ & 0x02))  // TH of port A as output.
    v = static_cast<uint8_t>((v & ~0x40) | (((ioControl_ >> 5) & 1) << 6));
  if (!(ioControl_ & 0x08))  // TH of port B as output.
    v = static_cast<uint8_t>((v & ~0x80) | (((ioControl_ >> 7) & 1) << 7));
  return v;
}

uint8_t MasterSystemBus::readPort(uint16_t port) {
  const uint8_t p = static_cast<uint8_t>(port & 0xFF);
  if (isGameGear() && p <= 0x06) {
    // Port 0: bit 7 = Start (active low), bit 6 = export region.
    return p == 0x00 ? static_cast<uint8_t>(start_ ? 0x7F : 0xFF) : 0xFF;
  }
  if (p < 0x40) return 0xFF;
  if (p < 0x80) return (p & 1) ? vdp_.readHCounter() : vdp_.readVCounter();
  if (p < 0xC0) return (p & 1) ? vdp_.readStatus() : vdp_.readData();
  return (p & 1) ? joypadB() : joypadA();
}

void MasterSystemBus::writePort(uint16_t port, uint8_t value) {
  const uint8_t p = static_cast<uint8_t>(port & 0xFF);
  if (isGameGear() && p == 0x06) {
    psg_.writeStereo(value);
    return;
  }
  if (p < 0x40) {
    if (p & 1) {
      ioControl_ = value;
    } else {
      memoryControl_ = value;
    }
    return;
  }
  if (p < 0x80) {
    psg_.write(value);
    return;
  }
  if (p < 0xC0) {
    if (p & 1) {
      vdp_.writeControl(value);
    } else {
      vdp_.writeData(value);
    }
  }
}

void MasterSystemBus::setButton(int player, Button button, bool pressed) {
  if (player < 0 || player > 1) return;
  const uint8_t bit = static_cast<uint8_t>(1u << button);
  auto &p = pad_[static_cast<size_t>(player)];
  p = pressed ? static_cast<uint8_t>(p | bit) : static_cast<uint8_t>(p & ~bit);
}

void MasterSystemBus::setPause(bool pressed) {
  if (pressed && !pause_) cpu_.requestNMI();
  pause_ = pressed;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

MasterSystemBusState MasterSystemBus::getState() const {
  MasterSystemBusState s;
  s.cpu = cpu_.getState();
  s.ram = ram_;
  s.vdp = vdp_.getState();
  s.psg = psg_.getState();
  s.cartridge = cartridge_.getState();
  s.memoryControl = memoryControl_;
  s.ioControl = ioControl_;
  s.pad = pad_;
  s.start = start_;
  s.pause = pause_;
  s.cpuCycles = cpuCycles_;
  return s;
}

void MasterSystemBus::setState(const MasterSystemBusState &s) {
  cpu_.setState(s.cpu);
  ram_ = s.ram;
  vdp_.setState(s.vdp);
  psg_.setState(s.psg);
  cartridge_.setState(s.cartridge);
  memoryControl_ = s.memoryControl;
  ioControl_ = s.ioControl;
  pad_ = s.pad;
  start_ = s.start;
  pause_ = s.pause;
  cpuCycles_ = s.cpuCycles;
  frameReady_ = false;
}
