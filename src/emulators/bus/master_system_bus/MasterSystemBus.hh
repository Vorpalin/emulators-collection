#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "emulators/audio/sega_psg/SegaPSG.hh"
#include "emulators/cartridge/sega_cartridge/SegaCartridge.hh"
#include "emulators/cpu/Z80/Z80.hh"
#include "emulators/cpu/Z80/Z80Bus.hh"
#include "emulators/ppu/sega_vdp/SegaVDP.hh"

/**
 * @file MasterSystemBus.hh
 * @brief Sega Master System / Game Gear system bus: owns the Z80, VDP, PSG,
 *        RAM and cartridge and routes memory and I/O accesses.
 */

/** @brief Serializable state of the whole machine (everything but the ROM). */
struct MasterSystemBusState {
  Z80State cpu;
  std::array<uint8_t, 0x2000> ram{};
  SegaVDPState vdp;
  SegaPSGState psg;
  SegaCartridgeState cartridge;
  uint8_t memoryControl = 0xAB;
  uint8_t ioControl = 0xFF;
  std::array<uint8_t, 2> pad{};
  bool start = false;
  bool pause = false;
  uint64_t cpuCycles = 0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(MasterSystemBusState, cpu, ram, vdp, psg,
                                   cartridge, memoryControl, ioControl, pad,
                                   start, pause, cpuCycles)

/**
 * @class MasterSystemBus
 * @brief Memory map and I/O ports of the Sega Master System (and Game Gear).
 *
 * **Memory map**
 *
 * | Range         | Device                                                 |
 * | ------------- | ------------------------------------------------------ |
 * | 0x0000-0xBFFF | Cartridge, three 16 KiB banked slots                   |
 * | 0xC000-0xDFFF | 8 KiB system RAM                                       |
 * | 0xE000-0xFFFF | Mirror of the RAM (0xFFFC-0xFFFF also hold mapper regs)|
 *
 * **I/O ports** (decoded on the low 8 bits; address bit 6/7 select the block)
 *
 * | Ports     | Read                         | Write                       |
 * | --------- | ---------------------------- | --------------------------- |
 * | 0x00-0x3F | (Game Gear: 0x00 Start)      | even: memory ctrl (0x3E), odd: I/O ctrl (0x3F), GG 0x06: PSG stereo |
 * | 0x40-0x7F | even: V counter, odd: H counter | PSG                      |
 * | 0x80-0xBF | even: VDP data, odd: VDP status | even: VDP data, odd: VDP control |
 * | 0xC0-0xFF | even: joypad A (0xDC), odd: joypad B (0xDD) | -          |
 *
 * There is no BIOS: the machine starts in the state the BIOS leaves it in
 * (cartridge mapped, SP = 0xDFF0, PC = 0). The VDP /INT line drives the Z80
 * interrupt input; the Pause button triggers an NMI.
 */
class MasterSystemBus : public Z80Bus {
 public:
  /** @brief Joypad buttons (bit index in the per-player mask). */
  enum Button : int {
    kUp = 0,
    kDown = 1,
    kLeft = 2,
    kRight = 3,
    kButton1 = 4,
    kButton2 = 5,
  };

  explicit MasterSystemBus(SegaVDP::Model model = SegaVDP::Model::MasterSystem);
  ~MasterSystemBus() override = default;
  MasterSystemBus(const MasterSystemBus &) = delete;
  MasterSystemBus &operator=(const MasterSystemBus &) = delete;

  /** @brief Loads a ROM image from memory, then resets the machine. */
  bool loadROM(const uint8_t *data, std::size_t size);
  /** @brief Loads a ROM file from disk, then resets the machine. */
  void loadROM(std::string &filename) override;
  const std::string &error() const { return cartridge_.error(); }

  /** @brief Resets the machine to its post-BIOS state (RAM cleared). */
  void reset() override;
  /** @brief Same as step(); satisfies the Bus interface. */
  void tick() override { step(); }

  /**
   * @brief Runs one Z80 instruction (or interrupt entry) and advances the VDP
   *        and PSG accordingly.
   * @return Z80 cycles elapsed.
   */
  uint32_t step();
  /** @brief True once per frame, when the VDP enters vertical blanking. */
  bool consumeFrameReady() {
    bool r = frameReady_;
    frameReady_ = false;
    return r;
  }

  uint8_t read(uint16_t address) override;
  void write(uint16_t address, uint8_t value) override;
  uint8_t readPort(uint16_t port) override;
  void writePort(uint16_t port, uint8_t value) override;

  /** @brief Presses or releases a joypad button (player 0 or 1). */
  void setButton(int player, Button button, bool pressed);
  /** @brief Presses or releases the Pause button (NMI on press). */
  void setPause(bool pressed);
  /** @brief Presses or releases the Game Gear Start button. */
  void setStart(bool pressed) { start_ = pressed; }

  bool isGameGear() const { return model_ == SegaVDP::Model::GameGear; }
  /** @brief Selects 50 Hz (PAL) or 60 Hz (NTSC) timing for VDP and PSG. */
  void setPAL(bool pal);
  uint64_t cpuCycles() const { return cpuCycles_; }

  Z80 &cpu() { return cpu_; }
  const Z80 &cpu() const { return cpu_; }
  SegaVDP &vdp() { return vdp_; }
  const SegaVDP &vdp() const { return vdp_; }
  SegaPSG &psg() { return psg_; }
  SegaCartridge &cartridge() { return cartridge_; }
  const SegaCartridge &cartridge() const { return cartridge_; }
  const std::array<uint8_t, 0x2000> &ram() const { return ram_; }

  MasterSystemBusState getState() const;
  void setState(const MasterSystemBusState &state);

 private:
  uint8_t joypadA() const;
  uint8_t joypadB() const;

  SegaVDP::Model model_;
  SegaCartridge cartridge_;
  SegaVDP vdp_;
  SegaPSG psg_;
  std::array<uint8_t, 0x2000> ram_{};
  Z80 cpu_;
  uint8_t memoryControl_ = 0xAB;
  uint8_t ioControl_ = 0xFF;
  std::array<uint8_t, 2> pad_{};
  bool start_ = false;
  bool pause_ = false;
  bool frameReady_ = false;
  uint64_t cpuCycles_ = 0;
};
