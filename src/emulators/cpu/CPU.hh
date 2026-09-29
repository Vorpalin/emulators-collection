#pragma once

#include <cstdint>

/**
 * @file CPU.hh
 * @brief Interface for an emulated CPU core.
 */

/**
 * @class CPU
 * @brief Abstract interface for an emulated processor: fetch/decode/execute
 *        cycle, reset, and memory access with cycle accounting.
 */
class CPU {
 public:
  /**
   * @brief Execute the next instruction.
   * @return The number of cycles the executed instruction consumed.
   */
  virtual uint32_t execute() = 0;

  /**
   * @brief Reset the CPU to its initial power-up state.
   */
  virtual void reset() = 0;

  /**
   * @brief Read a byte from memory via the bus, accounting for elapsed
   *        cycles.
   * @param cycles  Running cycle counter, incremented by this access.
   * @param address Address to read from.
   * @return The byte value at that address.
   */
  virtual uint8_t readMemory(uint32_t &cycles, uint16_t address) = 0;

  /**
   * @brief Write a byte to memory via the bus, accounting for elapsed
   *        cycles.
   * @param cycles  Running cycle counter, incremented by this access.
   * @param address Address to write to.
   * @param value   Byte value to write.
   */
  virtual void writeMemory(uint32_t &cycles, uint16_t address,
                           uint8_t value) = 0;
};
