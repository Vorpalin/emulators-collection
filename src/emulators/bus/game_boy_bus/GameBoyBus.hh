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

/**
 * @brief State of the Game Boy bus, for serialization/deserialization.
 * @details This structure holds the state of all components in the Game Boy bus
 *          for the purpose of saving and loading the emulator's state.
 */
struct GameBoyBusState {
  LR35902State cpu;
  GameBoyInterruptControllerState interrupts;
  APUState apu;
  GameBoyPPUState ppu;
  GameBoyTimerState timer;
  GameBoyControllerState joypad;

  std::array<uint8_t, 0x2000> wram;
  std::array<uint8_t, 0x7F> hram;

  uint8_t serialData;
  uint8_t serialControl;

  bool dmaActive;
  uint16_t dmaSource = 0;
  uint16_t dmaIndex = 0;
  uint8_t dmaDotCounter = 0;
  bool dmaJustStarted = false;

  // --- Game Boy Color extension (absent from pre-CGB save states) ---
  bool cgbMode = false;
  bool doubleSpeed = false;
  bool speedSwitchArmed = false;
  uint8_t svbk = 1;
  std::array<uint8_t, 0x6000> wramExt{};  ///< WRAM banks 2-7.
  uint16_t hdmaSource = 0;
  uint16_t hdmaDest = 0;
  uint8_t hdmaRemaining = 0;  ///< 16-byte blocks left.
  bool hdmaActive = false;    ///< HBlank DMA in progress.
};

/**
 * @brief Serialize the GameBoyBusState to JSON.
 * @param j     JSON object to populate.
 * @param state State to serialize.
 */
void to_json(nlohmann::json& j, const GameBoyBusState& state);
/**
 * @brief Deserialize the GameBoyBusState from JSON.
 * @param j     JSON object to read from.
 * @param state State to populate.
 */
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
 * | 0xFF4C-0xFF7F   | Unused / CGB registers (see below)   |
 * | 0xFF80-0xFFFE   | High RAM                             |
 * | 0xFFFF          | Interrupt Enable register            |
 *
 * Game Boy Color (enabled automatically when the cartridge header says so):
 * KEY1 (0xFF4D) speed switch, VBK (0xFF4F) VRAM bank, HDMA1-5 (0xFF51-0xFF55),
 * BCPS/BCPD/OCPS/OCPD/OPRI (0xFF68-0xFF6C, in the PPU) and SVBK (0xFF70) which
 * banks 0xD000-0xDFFF over 7 WRAM banks.
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
    if (!cartridge.loadROM(data, size)) return false;
    configureHardware();
    return true;
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
   * @return Elapsed time in T-cycles of the fixed 4.194304 MHz clock. In CGB
   *         double-speed mode the CPU runs twice as fast, so an instruction
   *         counts for half as much real time: this keeps the video/audio
   *         pacing of the caller independent of the CPU speed.
   */
  uint32_t step();

  /**
   * @brief Switches CPU speed when a STOP is about to execute.
   * @details Called by step(). On a Game Boy Color with KEY1 bit 0 armed, this
   *          toggles between normal and double speed.
   */
  void onStop();

  /** @brief True when running in Game Boy Color mode. */
  bool cgbMode() const { return cgbMode_; }
  /** @brief True when the CGB CPU is in double-speed mode. */
  bool doubleSpeed() const { return doubleSpeed_; }

  /** @brief CGB RGB555 framebuffer (valid when cgbMode() is true). */
  const std::array<uint16_t, 160 * 144>& getColorFramebuffer() const {
    return ppu.colorFramebuffer();
  }

  /**
   * @brief Bytes the program has sent over the serial port (SB/SC).
   * @details Every write of 0x81 (start, internal clock) to SC appends SB.
   *          Nothing is ever shifted in; test ROMs (e.g. Blargg's) use this to
   *          print their result. Capped at 64 KiB and cleared by reset().
   */
  const std::string& serialOutput() const { return serialOutput_; }

  /** @brief Gives access to the cartridge (battery RAM, header info). */
  GameBoyCartridge& getCartridge() { return cartridge; }
  const GameBoyCartridge& getCartridge() const { return cartridge; }

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

  /**
   * @brief Set the current state of the bus from a previously saved state.
   * @param state State to restore.
   */
  void setState(const GameBoyBusState& state);

  /**
   * @brief Get the current state of the bus for serialization/debugging.
   * @return Current state of the bus.
   */
  GameBoyBusState getState() const;

 private:
  /**
   * @brief Starts an OAM DMA transfer (write to 0xFF46).
   * @param sourceHigh High byte of the source address; 160 bytes are copied
   *        from `sourceHigh << 8` into OAM.
   */
  void startOamDma(uint8_t sourceHigh);
  void tickOamDma(uint32_t dots);

  /**
   * @brief Picks DMG or CGB mode from the cartridge header and applies the
   *        matching power-on state (CPU registers, banks, DMA, speed).
   */
  void configureHardware();

  /** @brief WRAM byte for an offset in 0x0000-0x1FFF (SVBK-banked on CGB). */
  uint8_t& wramAt(uint16_t offset);

  /** @brief Copies one 16-byte HDMA block into VRAM and stalls the CPU. */
  void hdmaCopyBlock();
  /** @brief Handles a write to HDMA5 (0xFF55). */
  void writeHdma5(uint8_t value);
  /** @brief Value read from HDMA5 (0xFF55). */
  uint8_t readHdma5() const;

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
  std::string serialOutput_;  ///< Serial bytes sent so far (diagnostics only).

  // --- Game Boy Color ---
  bool cgbMode_ = false;                ///< Running a CGB-aware cartridge.
  bool doubleSpeed_ = false;            ///< KEY1 bit 7.
  bool speedSwitchArmed_ = false;       ///< KEY1 bit 0.
  uint8_t svbk_ = 1;                    ///< Selected WRAM bank (0xD000-0xDFFF).
  std::array<uint8_t, 0x6000> wramExt;  ///< WRAM banks 2-7 (bank 1 is wram).
  uint16_t hdmaSource_ = 0;
  uint16_t hdmaDest_ = 0;      ///< Offset in VRAM (0-0x1FF0).
  uint8_t hdmaRemaining_ = 0;  ///< 16-byte blocks left.
  bool hdmaActive_ = false;    ///< HBlank DMA pending.
  uint32_t stallCycles_ = 0;   ///< M-cycles the CPU is stalled by DMA.

  bool dmaActive = false;  ///< True while an OAM DMA transfer is running.
  uint16_t dmaSource_ = 0;
  uint16_t dmaIndex_ = 0;
  uint8_t dmaDotCounter_ = 0;
  bool dmaJustStarted_ = false;
  bool dmaReadInProgress_ = false;
  bool frameReady = false;  ///< Set by the PPU when a frame is complete.
};
