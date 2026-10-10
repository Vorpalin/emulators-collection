#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

#include "emulators/cpu/CPU65/CPU65.hh"

struct NesBusState {
  CPU65State cpu;
};

/**
 * @brief Serialize the NesBusState to JSON.
 * @param j     JSON object to populate.
 * @param state State to serialize.
 */
void to_json(nlohmann::json& j, const NesBusState& state);
/**
 * @brief Deserialize the NesBusState from JSON.
 * @param j     JSON object to read from.
 * @param state State to populate.
 */
void from_json(const nlohmann::json& j, NesBusState& state);

class NesBus : public Bus {
 public:
  /** @brief Constructs the bus and all attached components. */
  NesBus();
  ~NesBus();

  /** @brief Resets all components to their power-on state. */
  void reset() override;

  /**
   * @brief Loads a ROM into the cartridge.
   * @param filename Path to the ROM file.
   */
  void loadROM(std::string& filename) override;

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
   * @brief Set the current state of the bus from a previously saved state.
   * @param state State to restore.
   */
  void setState(const NesBusState& state);

  /**
   * @brief Get the current state of the bus for serialization/debugging.
   * @return Current state of the bus.
   */
  NesBusState getState() const;

 private:
  CPU65 cpu;  ///< MOS Technology 6502 CPU.
};
