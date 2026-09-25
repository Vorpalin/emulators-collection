#pragma once
#include <string>

/**
 * @file Bus.hh
 * @brief Interface for a system bus connecting a CPU to memory and other
 *        memory-mapped devices.
 */

/**
 * @class Bus
 * @brief Abstract interface for an emulated system bus: handles ROM
 *        loading, reset, timing ticks and memory-mapped reads/writes.
 */
class Bus {
 public:
  /**
   * @brief Load a ROM file into the system's memory.
   * @param filename Path to the ROM file to load.
   */
  virtual void loadROM(std::string &filename) = 0;

  /**
   * @brief Reset the bus and any connected devices to their initial state.
   */
  virtual void reset() = 0;

  /**
   * @brief Advance the bus and connected devices by one timing step.
   */
  virtual void tick() = 0;

  /**
   * @brief Read a byte from the given memory-mapped address.
   * @param address Address to read from.
   * @return The byte value at that address.
   */
  virtual uint8_t readMemory(uint16_t address) = 0;

  /**
   * @brief Write a byte to the given memory-mapped address.
   * @param address Address to write to.
   * @param value   Byte value to write.
   */
  virtual void writeMemory(uint16_t address, uint8_t value) = 0;
};
