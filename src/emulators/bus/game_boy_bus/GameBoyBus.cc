#include "emulators/bus/game_boy_bus/GameBoyBus.hh"

#include <algorithm>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void to_json(nlohmann::json& j, const GameBoyBusState& state) {
  j = {
      {"cpu", state.cpu},
      {"interrupts", state.interrupts},
      {"apu", state.apu},
      {"ppu", state.ppu},
      {"timer", state.timer},
      {"joypad", state.joypad},

      {"wram", state.wram},
      {"hram", state.hram},

      {"serialData", state.serialData},
      {"serialControl", state.serialControl},

      {"dmaActive", state.dmaActive},

      {"cgbMode", state.cgbMode},
      {"doubleSpeed", state.doubleSpeed},
      {"speedSwitchArmed", state.speedSwitchArmed},
      {"svbk", state.svbk},
      {"wramExt", state.wramExt},
      {"hdmaSource", state.hdmaSource},
      {"hdmaDest", state.hdmaDest},
      {"hdmaRemaining", state.hdmaRemaining},
      {"hdmaActive", state.hdmaActive},
  };
}

void from_json(const nlohmann::json& j, GameBoyBusState& state) {
  j.at("cpu").get_to(state.cpu);
  j.at("interrupts").get_to(state.interrupts);
  j.at("apu").get_to(state.apu);
  j.at("ppu").get_to(state.ppu);
  j.at("timer").get_to(state.timer);
  j.at("joypad").get_to(state.joypad);

  j.at("wram").get_to(state.wram);
  j.at("hram").get_to(state.hram);

  j.at("serialData").get_to(state.serialData);
  j.at("serialControl").get_to(state.serialControl);

  j.at("dmaActive").get_to(state.dmaActive);

  // Game Boy Color fields: optional so that DMG-era save states still load.
  state.cgbMode = j.value("cgbMode", false);
  state.doubleSpeed = j.value("doubleSpeed", false);
  state.speedSwitchArmed = j.value("speedSwitchArmed", false);
  state.svbk = j.value("svbk", static_cast<uint8_t>(1));
  if (j.contains("wramExt")) j.at("wramExt").get_to(state.wramExt);
  state.hdmaSource = j.value("hdmaSource", static_cast<uint16_t>(0));
  state.hdmaDest = j.value("hdmaDest", static_cast<uint16_t>(0));
  state.hdmaRemaining = j.value("hdmaRemaining", static_cast<uint8_t>(0));
  state.hdmaActive = j.value("hdmaActive", false);
}

void GameBoyBus::setState(const GameBoyBusState& state) {
  cpu.setState(state.cpu);
  interrupts.setState(state.interrupts);
  apu.setState(state.apu);
  ppu.setState(state.ppu);
  timer.setState(state.timer);
  joypad.setState(state.joypad);

  wram = state.wram;
  hram = state.hram;

  serialData = state.serialData;
  serialControl = state.serialControl;

  dmaActive = state.dmaActive;

  cgbMode_ = state.cgbMode;
  doubleSpeed_ = state.doubleSpeed;
  speedSwitchArmed_ = state.speedSwitchArmed;
  svbk_ = state.svbk;
  wramExt = state.wramExt;
  hdmaSource_ = state.hdmaSource;
  hdmaDest_ = state.hdmaDest;
  hdmaRemaining_ = state.hdmaRemaining;
  hdmaActive_ = state.hdmaActive;
  stallCycles_ = 0;
  ppu.setCgbMode(cgbMode_);  // also restored by ppu.setState()
}

GameBoyBusState GameBoyBus::getState() const {
  return {
      .cpu = cpu.getState(),
      .interrupts = interrupts.getState(),
      .apu = apu.getState(),
      .ppu = ppu.getState(),
      .timer = timer.getState(),
      .joypad = joypad.getState(),

      .wram = wram,
      .hram = hram,

      .serialData = serialData,
      .serialControl = serialControl,

      .dmaActive = dmaActive,

      .cgbMode = cgbMode_,
      .doubleSpeed = doubleSpeed_,
      .speedSwitchArmed = speedSwitchArmed_,
      .svbk = svbk_,
      .wramExt = wramExt,
      .hdmaSource = hdmaSource_,
      .hdmaDest = hdmaDest_,
      .hdmaRemaining = hdmaRemaining_,
      .hdmaActive = hdmaActive_,
  };
}

GameBoyBus::GameBoyBus()
    : cpu(this), cartridge(), ppu(), timer(&interrupts), joypad(&interrupts) {
  for (auto& byte : wram) {
    byte = 0;
  }
  for (auto& byte : hram) {
    byte = 0;
  }
  wramExt.fill(0);
  ppu.onVBlankInterrupt(
      [this]() { interrupts.request(GameBoyInterruptController::VBlank); });
  ppu.onStatInterrupt(
      [this]() { interrupts.request(GameBoyInterruptController::LCDStat); });
  ppu.onFrameReady([this]() { frameReady = true; });
  // CGB HBlank DMA: one 16-byte block per HBlank.
  ppu.onHBlank([this]() {
    if (!hdmaActive_) return;
    hdmaCopyBlock();
    if (hdmaRemaining_ == 0) hdmaActive_ = false;
  });
}

GameBoyBus::~GameBoyBus() {}

void GameBoyBus::loadROM(std::string& filename) {
  cartridge.loadROM(filename);
  configureHardware();
}

void GameBoyBus::configureHardware() {
  // CGB flag in the header: 0x80 = DMG + CGB, 0xC0 = CGB only. (An empty
  // cartridge reads 0xFF, which is neither.)
  const uint8_t cgbFlag = cartridge.read(0x0143);
  cgbMode_ = cgbFlag == 0x80 || cgbFlag == 0xC0;
  ppu.setCgbMode(cgbMode_);

  doubleSpeed_ = false;
  speedSwitchArmed_ = false;
  svbk_ = 1;
  wramExt.fill(0);
  hdmaSource_ = 0;
  hdmaDest_ = 0;
  hdmaRemaining_ = 0;
  hdmaActive_ = false;
  stallCycles_ = 0;

  // Register values the boot ROM leaves behind. Games detect a Game Boy Color
  // by checking that A == 0x11 at 0x0100.
  LR35902State regs = cpu.getState();
  if (cgbMode_) {
    regs.A = 0x11;
    regs.F = 0x80;
    regs.B = 0x00;
    regs.C = 0x00;
    regs.D = 0xFF;
    regs.E = 0x56;
    regs.H = 0x00;
    regs.L = 0x0D;
  } else {
    regs.A = 0x01;
    regs.F = 0xB0;
    regs.B = 0x00;
    regs.C = 0x13;
    regs.D = 0x00;
    regs.E = 0xD8;
    regs.H = 0x01;
    regs.L = 0x4D;
  }
  cpu.setState(regs);
}

uint8_t& GameBoyBus::wramAt(uint16_t offset) {
  offset &= 0x1FFF;
  if (offset < 0x1000) return wram[offset];  // bank 0: fixed
  unsigned bank = cgbMode_ ? (svbk_ & 0x07u) : 1u;
  if (bank == 0) bank = 1;  // SVBK = 0 selects bank 1
  if (bank == 1) return wram[offset];
  return wramExt[(bank - 2) * 0x1000 + (offset - 0x1000)];
}

void GameBoyBus::onStop() {
  if (!cgbMode_ || !speedSwitchArmed_) return;
  doubleSpeed_ = !doubleSpeed_;
  speedSwitchArmed_ = false;
  timer.write(0xFF04, 0);  // the speed switch resets DIV
}

void GameBoyBus::hdmaCopyBlock() {
  for (uint16_t i = 0; i < 16; ++i) {
    const uint16_t src = static_cast<uint16_t>(hdmaSource_ + i);
    // VRAM and the 0xE000+ region are not valid HDMA sources.
    const bool valid = src < 0x8000 || (src >= 0xA000 && src < 0xE000);
    const uint8_t value = valid ? read(src) : 0xFF;
    ppu.writeVRAMDMA(static_cast<uint16_t>((hdmaDest_ + i) & 0x1FFF),
                     value);
  }
  hdmaSource_ = static_cast<uint16_t>(hdmaSource_ + 16);
  hdmaDest_ = static_cast<uint16_t>((hdmaDest_ + 16) & 0x1FFF);
  if (hdmaRemaining_ > 0) --hdmaRemaining_;
  // The CPU is halted while the block is copied (8 M-cycles at normal speed).
  stallCycles_ += doubleSpeed_ ? 16 : 8;
}

void GameBoyBus::writeHdma5(uint8_t value) {
  if (hdmaActive_ && !(value & 0x80)) {
    hdmaActive_ = false;  // writing bit 7 = 0 cancels a running HBlank DMA
    return;
  }
  hdmaRemaining_ = static_cast<uint8_t>((value & 0x7F) + 1);
  if (value & 0x80) {
    hdmaActive_ = true;
    // Started during HBlank: the first block goes immediately.
    const auto& r = ppu.registers();
    if (r.lcdEnabled() && r.mode() == HBlank && r.ly < 144) {
      hdmaCopyBlock();
      if (hdmaRemaining_ == 0) hdmaActive_ = false;
    }
  } else {
    while (hdmaRemaining_ > 0) hdmaCopyBlock();  // general-purpose DMA
  }
}

uint8_t GameBoyBus::readHdma5() const {
  return static_cast<uint8_t>(((hdmaRemaining_ - 1) & 0x7F) |
                              (hdmaActive_ ? 0x00 : 0x80));
}

void GameBoyBus::reset() {
  cpu.reset();
  cartridge.reset();
  ppu.reset();
  apu.reset();
  timer.reset();
  joypad.reset();
  interrupts.reset();
  wram.fill(0);
  hram.fill(0);
  serialData = 0;
  serialControl = 0;
  serialOutput_.clear();
  dmaActive = false;
  configureHardware();  // DMG vs CGB depends on the loaded cartridge
}

void GameBoyBus::tick() { step(); }

uint32_t GameBoyBus::step() {
  // STOP (0x10) with KEY1 armed toggles the CGB speed. The CPU core treats
  // STOP as a 2-byte NOP, so the switch is detected here, just before the
  // instruction runs.
  if (cgbMode_ && speedSwitchArmed_ && read(cpu.getPC()) == 0x10) onStop();

  // Execute the instruction first, then advance the peripherals. HBlank DMA
  // can be started by a PPU callback while the peripherals are ticking, so
  // its CPU stall must be picked up by the same step rather than allowing an
  // extra CPU instruction on the next call.
  uint32_t remaining = cpu.execute() * 4;
  uint32_t elapsed = 0;  // in 4.194304 MHz "real time" T-cycles

  // General-purpose DMA may already have queued stalls during the instruction;
  // HBlank DMA may queue more from inside ppu.tick(). Drain both sources.
  remaining += stallCycles_ * 4;
  stallCycles_ = 0;

  // In double-speed mode the CPU and timer run twice as fast, while the PPU
  // and APU keep their normal clock. Keep chunks small enough for the
  // peripherals' 8-bit tick() interfaces.
  while (remaining > 0) {
    const uint32_t chunk = std::min<uint32_t>(remaining, 64);
    const uint32_t dots = doubleSpeed_ ? chunk / 2 : chunk;
    ppu.tick(static_cast<uint8_t>(dots));
    timer.tick(static_cast<uint8_t>(chunk));
    apu.tick(dots);
    remaining -= chunk;
    elapsed += dots;

    // The PPU can invoke the HBlank DMA callback above. Account for its stall
    // immediately, and allow the additional peripheral time to trigger any
    // further callbacks before returning to the CPU.
    if (stallCycles_ > 0) {
      remaining += stallCycles_ * 4;
      stallCycles_ = 0;
    }
  }
  return elapsed;
}

float GameBoyBus::getSample() { return apu.getSample(); }

void GameBoyBus::getStereoSample(float& left, float& right) {
  apu.getStereoSample(left, right);
}

void GameBoyBus::write(uint16_t address, uint8_t value) {
  if (address < 0x8000) {
    cartridge.write(address, value);
  } else if (address < 0xA000) {
    ppu.write(address, value);  // VRAM
  } else if (address < 0xC000) {
    cartridge.write(address, value);  // external/save RAM
  } else if (address < 0xE000) {
    wramAt(address) = value;
  } else if (address < 0xFE00) {
    wramAt(address) = value;  // echo RAM mirrors 0xC000-0xDDFF
  } else if (address < 0xFEA0) {
    ppu.write(address, value);  // OAM
  } else if (address < 0xFF00) {
    // Unusable region; real hardware mostly ignores writes here.
  } else if (address >= 0xFF10 && address < 0xFF40) {
    apu.write(address, value);
  } else if (address < 0xFF80) {
    switch (address) {
      case 0xFF00:
        joypad.write(value);
        break;
      case 0xFF01:
        serialData = value;
        break;
      case 0xFF02:
        serialControl = value;
        if ((value & 0x81) == 0x81 && serialOutput_.size() < 0x10000) {
          serialOutput_.push_back(static_cast<char>(serialData));
        }
        break;
      case 0xFF04:
      case 0xFF05:
      case 0xFF06:
      case 0xFF07:
        timer.write(address, value);
        break;
      case 0xFF0F:
        interrupts.writeIF(value);
        break;
      case 0xFF46:
        startOamDma(value);
        break;
      case 0xFF4D:  // KEY1: arm the speed switch (CGB only)
        if (cgbMode_) speedSwitchArmed_ = (value & 0x01) != 0;
        break;
      case 0xFF51:  // HDMA1: source high
        if (cgbMode_)
          hdmaSource_ =
              static_cast<uint16_t>((hdmaSource_ & 0x00FF) | (value << 8));
        break;
      case 0xFF52:  // HDMA2: source low (low 4 bits ignored)
        if (cgbMode_)
          hdmaSource_ =
              static_cast<uint16_t>((hdmaSource_ & 0xFF00) | (value & 0xF0));
        break;
      case 0xFF53:  // HDMA3: destination high (VRAM offset, 13 bits)
        if (cgbMode_)
          hdmaDest_ = static_cast<uint16_t>((hdmaDest_ & 0x00FF) |
                                            ((value & 0x1F) << 8));
        break;
      case 0xFF54:  // HDMA4: destination low
        if (cgbMode_)
          hdmaDest_ =
              static_cast<uint16_t>((hdmaDest_ & 0xFF00) | (value & 0xF0));
        break;
      case 0xFF55:  // HDMA5: start / cancel a VRAM DMA
        if (cgbMode_) writeHdma5(value);
        break;
      case 0xFF70:  // SVBK: WRAM bank for 0xD000-0xDFFF
        if (cgbMode_) svbk_ = value & 0x07;
        break;
      default:
        ppu.write(address, value);
        break;  // APU/PPU registers
    }
  } else if (address < 0xFFFF) {
    hram[address - 0xFF80] = value;
  } else {
    interrupts.writeIE(value);
  }
}

uint8_t GameBoyBus::read(uint16_t address) {
  if (address < 0x8000) {
    return cartridge.read(address);
  } else if (address < 0xA000) {
    return ppu.read(address);  // VRAM
  } else if (address < 0xC000) {
    return cartridge.read(address);  // external/save RAM
  } else if (address < 0xE000) {
    return wramAt(address);
  } else if (address < 0xFE00) {
    return wramAt(address);  // echo RAM
  } else if (address < 0xFEA0) {
    return ppu.read(address);  // OAM
  } else if (address < 0xFF00) {
    return 0xFF;  // unusable region
  } else if (address >= 0xFF10 && address < 0xFF40) {
    return apu.read(address);
  } else if (address < 0xFF80) {
    switch (address) {
      case 0xFF00:
        return joypad.read();
      case 0xFF01:
        return serialData;
      case 0xFF02:
        return serialControl;
      case 0xFF04:
      case 0xFF05:
      case 0xFF06:
      case 0xFF07:
        return timer.read(address);
      case 0xFF0F:
        return interrupts.readIF();
      case 0xFF4D:  // KEY1
        if (!cgbMode_) return 0xFF;
        return static_cast<uint8_t>(0x7E | (doubleSpeed_ ? 0x80 : 0x00) |
                                    (speedSwitchArmed_ ? 0x01 : 0x00));
      case 0xFF55:  // HDMA5
        return cgbMode_ ? readHdma5() : 0xFF;
      case 0xFF70:  // SVBK
        return cgbMode_ ? static_cast<uint8_t>(0xF8 | (svbk_ & 0x07)) : 0xFF;
      default:
        return ppu.read(address);  // APU/PPU registers
    }
  } else if (address < 0xFFFF) {
    return hram[address - 0xFF80];
  } else {
    return interrupts.readIE();
  }
}

void GameBoyBus::startOamDma(uint8_t sourceHigh) {
  uint16_t base = static_cast<uint16_t>(sourceHigh) << 8;
  for (uint16_t i = 0; i < 0xA0; ++i) {
    ppu.writeOAMDMA(i, read(base + i));
  }
}
