#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

#include "emulators/audio/nes_apu/NesAPU.hh"
#include "emulators/bus/Bus.hh"
#include "emulators/cartridge/nes_cartridge/NesCartridge.hh"
#include "emulators/controller/nes_controller/NesController.hh"
#include "emulators/cpu/CPU65/CPU65.hh"
#include "emulators/ppu/PPU2C02/PPU2C02.hh"

/**
 * @file NesBus.hh
 * @brief NES system bus: owns the CPU, PPU, APU, controllers and cartridge.
 */

/** Serializable state of the whole NES (everything except the ROM data). */
struct NesBusState {
  CPU65State cpu;
  std::array<uint8_t, 0x800> ram;
  PPU2C02State ppu;
  NesAPUState apu;
  std::array<NesController, 2> pads;
  nlohmann::json cartridge;
  uint64_t cpu_cycles = 0;
  uint32_t dma_stall = 0;
  uint8_t open_bus = 0;
};

void to_json(nlohmann::json& j, const NesBusState& state);
void from_json(const nlohmann::json& j, NesBusState& state);

/**
 * @class NesBus
 * @brief CPU memory map of the NES.
 *
 * | Range         | Device                              |
 * | ------------- | ----------------------------------- |
 * | $0000-$1FFF   | 2 KiB RAM (mirrored)                |
 * | $2000-$3FFF   | PPU registers (mirrored every 8)    |
 * | $4000-$4017   | APU, OAM DMA, controllers           |
 * | $4020-$FFFF   | Cartridge (mapper)                  |
 */
class NesBus : public Bus {
 public:
  NesBus();
  ~NesBus() override = default;
  NesBus(const NesBus&) = delete;
  NesBus& operator=(const NesBus&) = delete;

  /** @brief Resets all components to their power-on state. */
  void reset() override;

  /** @brief Load an iNES file from disk, then reset. */
  void loadROM(std::string& filename) override;

  /**
   * @brief Load an iNES image from memory, then reset.
   * @return false if the image is invalid or its mapper is unsupported.
   */
  bool loadROM(const uint8_t* data, std::size_t size);

  /** @brief Reason of the last failed loadROM(). */
  const std::string& error() const { return cartridge_.error(); }

  uint8_t read(uint16_t address) override;
  void write(uint16_t address, uint8_t value) override;

  /** @brief Same as step(); satisfies the Bus interface. */
  void tick() override { step(); }

  /**
   * @brief Run one CPU instruction (or interrupt entry) and advance the PPU
   *        and APU accordingly.
   * @return CPU cycles elapsed.
   */
  uint32_t step();

  /** @brief True once per completed video frame. */
  bool consumeFrameReady() { return ppu_.consumeFrameReady(); }

  bool isHalted() const { return cpu_.getState().halted; }

  PPU2C02& ppu() { return ppu_; }
  const PPU2C02& ppu() const { return ppu_; }
  NesAPU& apu() { return apu_; }
  NesController& pad(std::size_t i) { return pads_[i]; }
  NesCartridge& cartridge() { return cartridge_; }
  const NesCartridge& cartridge() const { return cartridge_; }

  void setState(const NesBusState& state);
  NesBusState getState() const;

 private:
  void advance(uint32_t cpuCycles);
  uint8_t peek(uint16_t address);

  // Declaration order matters: the CPU reads the reset vector from this bus
  // when it is constructed, so everything it can touch comes first.
  NesCartridge cartridge_;
  PPU2C02 ppu_;
  NesAPU apu_;
  std::array<NesController, 2> pads_{};
  std::array<uint8_t, 0x800> ram_{};
  CPU65 cpu_;

  uint64_t cpu_cycles_ = 0;
  uint32_t dma_stall_ = 0;
  uint8_t open_bus_ = 0;
  bool irq_poll_delayed_ = false;  ///< Last opcode was CLI/SEI/PLP.
  bool prev_i_ = true;             ///< I flag before that instruction.
};
