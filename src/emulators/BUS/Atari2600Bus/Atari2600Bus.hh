#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include "Bus.hh"
#include "CPU65.hh"
#include "Cartbridge.hh"
#include "MOS6532.hh"
#include "TIA1A.hh"

/**
 * @file Atari2600Bus.hh
 * @brief System bus wiring together the Atari 2600's CPU, cartridge,
 *        RIOT and TIA into a single addressable machine.
 */

/**
 * @class Atari2600Bus
 * @brief Concrete Bus implementation for the Atari 2600: decodes CPU
 *        addresses to the cartridge, RIOT (MOS6532) or TIA (TIA1A), owns
 *        the CPU65 core, and drives per-cycle ticking of the TIA.
 */
class Atari2600Bus : public Bus {
 public:
  /**
   * @brief Construct the bus and its owned CPU/cartridge/RIOT/TIA components.
   */
  Atari2600Bus();

  /**
   * @brief Load a cartridge ROM image.
   * @param filename Path to the ROM file to load.
   */
  void loadROM(std::string& filename) override;

  /**
   * @brief Reset the CPU and all attached components to their power-up
   *        state.
   */
  void reset() override;

  /**
   * @brief Read a byte from the address decoded to cartridge, RIOT or TIA.
   * @param address Full 16-bit CPU address.
   * @return The byte value read.
   */
  uint8_t readMemory(uint16_t address) override;

  /**
   * @brief Write a byte to the address decoded to cartridge, RIOT or TIA.
   *        TIA audio-register writes also invoke the registered audio hook.
   * @param address Full 16-bit CPU address.
   * @param value   Byte value to write.
   */
  void writeMemory(uint16_t address, uint8_t value) override;

  /**
   * @brief Advance the bus (and the TIA) by one clock cycle.
   */
  void tick() override;

  /**
   * @brief Access the TIA's current rendered frame buffer.
   * @return Pointer to the pixel data (see TIA1A::frame()).
   */
  const uint8_t* frame() const { return tia1a.frame(); }

  /**
   * @brief Whether the TIA has completed a full frame.
   * @return True if a new frame is ready to be displayed.
   */
  bool frameReady() const { return tia1a.frameReady(); }

  /**
   * @brief Clear the TIA's frame-ready flag after the frame is consumed.
   */
  void clearFrameReady() { tia1a.clearFrameReady(); }

  /**
   * @brief Update the RIOT's switch inputs and the TIA's fire-button state.
   * @param swcha Value for Port A (SWCHA), e.g. joystick directions.
   * @param swchb Value for Port B (SWCHB), e.g. console switches.
   * @param fire0 Fire-button state for player 0.
   * @param fire1 Fire-button state for player 1.
   */
  void setInput(uint8_t swcha, uint8_t swchb, bool fire0, bool fire1);

  /**
   * @brief Register a callback invoked after every write to a TIA audio
   *        register (0x15..0x1A), e.g. to feed a TIAAudio instance.
   * @param hook Callback receiving the written address and value.
   */
  void setAudioWriteHook(std::function<void(uint16_t, uint8_t)> hook) {
    onAudioWrite = std::move(hook);
  }

 private:
  CPU65 cpu;              ///< CPU core driving the Atari 2600.
  Cartbridge cartbridge;  ///< Loaded ROM and bank-switching state.
  MOS6532 mos6532;        ///< RAM, I/O ports and timer.
  TIA1A tia1a;            ///< Video generation and CPU-halt (WSYNC) signaling.
  std::function<void(uint16_t, uint8_t)>
      onAudioWrite;  ///< Optional hook fired on TIA audio-register writes.
};
