#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

#include "emulators/audio/APU/APU.hh"
#include "emulators/bus/Bus.hh"
#include "emulators/cartridge/game_boy_cartridge/GameBoyCartridge.hh"
#include "emulators/controller/game_boy_controller/GameBoyController.hh"
#include "emulators/cpu/LR35902/LR35902.hh"
#include "emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.hh"
#include "emulators/ppu/game_boy_ppu/GameBoyPPU.hh"
#include "emulators/timer/game_boy_timer/GameBoyTimer.hh"

/**
 * @file GameBoyBus.hh
 * @brief Game Boy system bus: memory map and component wiring.
 */

struct GameBoyBusState {
  LR35902State cpu;
  GameBoyInterruptControllerState interrupts;
  APUState apu;
  GameBoyCartridgeState cartridge;
  GameBoyPPUState ppu;
  GameBoyTimerState timer;
  GameBoyControllerState joypad;

  std::array<uint8_t, 0x2000> wram;
  std::array<uint8_t, 0x7F> hram;

  uint8_t serialData;
  uint8_t serialControl;

  bool dmaActive;
};

void to_json(nlohmann::json& j, const GameBoyBusState& state);
void from_json(const nlohmann::json& j, GameBoyBusState& state);

/**
 * @class GameBoyBus
 * @brief Connects the CPU to memory and I/O and drives component timing.
 *
 * The bus owns the CPU, cartridge, PPU, timer, joypad, interrupt controller,
 * work RAM and high RAM, and routes every CPU access to the right component
 * according to the Game Boy memory map:
 *
 * | Range           | Content                              |
 * |-----------------|--------------------------------------|
 * | 0x0000-0x7FFF   | Cartridge ROM (banked)               |
 * | 0x8000-0x9FFF   | Video RAM (PPU)                      |
 * | 0xA000-0xBFFF   | Cartridge external RAM               |
 * | 0xC000-0xDFFF   | Work RAM                             |
 * | 0xE000-0xFDFF   | Echo of work RAM                     |
 * | 0xFE00-0xFE9F   | OAM (PPU)                            |
 * | 0xFEA0-0xFEFF   | Unusable                             |
 * | 0xFF00-0xFF0F   | I/O registers                        |
 * | 0xFF10-0xFF3F   | Audio registers                      |
 * | 0xFF40-0xFF4B   | PPU registers                        |
 * | 0xFF4C-0xFF7F   | Unused                               |
 * | 0xFF80-0xFFFE   | High RAM                             |
 * | 0xFFFF          | Interrupt Enable register            |
 */
class GameBoyBus : public Bus {
 public:
  /** @brief Constructs the bus and all attached components. */
  GameBoyBus();
  ~GameBoyBus();

  /**
   * @brief Loads a ROM into the cartridge.
   * @param filename Path to the ROM file.
   */
  void loadROM(std::string& filename) override;

  /**
   * @brief Load a ROM image from memory.
   * @param data Pointer to the ROM bytes.
   * @param size Number of bytes.
   * @return false if the cartridge rejects the image.
   */
  bool loadROM(const uint8_t* data, std::size_t size) {
    return cartridge.loadROM(data, size);
  }

  /** @brief Resets all components to their power-on state. */
  void reset() override;

  /**
   * @brief Reads a byte from the memory map.
   * @param address 16-bit bus address.
   * @return Byte at that address.
   */
  uint8_t read(uint16_t address) override;

  /**
   * @brief Writes a byte to the memory map.
   * @param address 16-bit bus address.
   * @param value   Byte to write.
   */
  void write(uint16_t address, uint8_t value) override;

  /**
   * @brief Advances the whole system (timer, PPU, DMA, ...) by one step.
   *
   * Typically called once per elapsed machine cycle by the CPU.
   */
  void tick() override;

  /**
   * @brief Executes one CPU instruction and advances PPU, timer and APU.
   * @return Number of T-cycles (4.194304 MHz clock) the instruction took.
   */
  uint32_t step();

  /** @brief Gives access to the joypad, to feed it host input. */
  GameBoyController& getJoypad() { return joypad; }

  /** @brief Returns the current 160x144 PPU framebuffer (shade per pixel). */
  const std::array<uint8_t, 160 * 144>& getFramebuffer() const {
    return ppu.framebuffer();
  }

  /**
   * @brief Tells whether a new frame has been completed since the last call,
   *        and clears the flag.
   * @return `true` if a frame is ready to be displayed.
   */
  bool consumeFrameReady() {
    bool r = frameReady;
    frameReady = false;
    return r;
  }

  /**
   * @brief Gets the current audio sample.
   * @return The current audio sample.
   */
  float getSample();

  /**
   * @brief Gets the current stereo audio sample (left/right, each in [-1, 1]).
   */
  void getStereoSample(float& left, float& right);

  /// Interrupt controller (IF at 0xFF0F, IE at 0xFFFF). Public so that the
  /// CPU and peripherals can request/query interrupts.
  GameBoyInterruptController interrupts;

  void setState(const GameBoyBusState& state);
  GameBoyBusState getState() const;

 private:
  /**
   * @brief Starts an OAM DMA transfer (write to 0xFF46).
   * @param sourceHigh High byte of the source address; 160 bytes are copied
   *        from `sourceHigh << 8` into OAM.
   */
  void startOamDma(uint8_t sourceHigh);

  LR35902 cpu;                       ///< Sharp LR35902 CPU.
  GameBoyCartridge cartridge;        ///< Cartridge (ROM + external RAM).
  GameBoyPPU ppu;                    ///< Picture processing unit.
  std::array<uint8_t, 0x2000> wram;  ///< Work RAM (0xC000-0xDFFF).
  std::array<uint8_t, 0x7F> hram;    ///< High RAM (0xFF80-0xFFFE).
  APU apu;                           ///< Audio processing unit.

  GameBoyTimer timer;        ///< DIV/TIMA/TMA/TAC timer.
  GameBoyController joypad;  ///< Joypad register (0xFF00).

  uint8_t serialData = 0;     ///< Serial transfer data (SB, 0xFF01).
  uint8_t serialControl = 0;  ///< Serial transfer control (SC, 0xFF02).

  bool dmaActive = false;   ///< True while an OAM DMA transfer is running.
  bool frameReady = false;  ///< Set by the PPU when a frame is complete.
};
