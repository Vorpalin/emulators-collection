#pragma once

#include <array>
#include <cstdint>

/**
 * @file MOS6532.hh
 * @brief MOS 6532 RIOT (RAM-I/O-Timer) emulation, as used by the Atari 2600
 *        for its 128 bytes of RAM, I/O ports and interval timer.
 */

/**
 * @class MOS6532
 * @brief Emulates the MOS 6532 RIOT chip: 128 bytes of RAM, two 8-bit
 *        bidirectional I/O ports (A and B) with data direction registers,
 *        and a programmable interval timer.
 */
class MOS6532 {
 public:
  /**
   * @brief Construct a MOS6532 and reset it to its power-up state.
   */
  MOS6532();

  /**
   * @brief Reset RAM, I/O ports, direction registers and the timer.
   */
  void reset();

  /**
   * @brief Read from a RIOT-mapped address (RAM, I/O port or timer register).
   * @param address Address, as decoded by the caller to fall within the
   *                RIOT's address range.
   * @return The byte read.
   */
  uint8_t read(uint16_t address);

  /**
   * @brief Write to a RIOT-mapped address (RAM, I/O port, direction
   *        register or timer register).
   * @param address Address, as decoded by the caller to fall within the
   *                RIOT's address range.
   * @param value   Byte value to write.
   */
  void write(uint16_t address, uint8_t value);

  /**
   * @brief Advance the internal timer by the given number of CPU cycles.
   * @param cpu_cycles Number of CPU cycles elapsed since the last update.
   */
  void update(int cpu_cycles);

  /**
   * @brief Set the input value presented on Port A (SWCHA), e.g. joystick
   *        switches.
   * @param v Port A input value.
   */
  void setSwcha(uint8_t v);

  /**
   * @brief Set the input value presented on Port B (SWCHB), e.g. console
   *        switches.
   * @param v Port B input value.
   */
  void setSwchb(uint8_t v);

 private:
  std::array<uint8_t, 128> ram;     ///< 128 bytes of RAM.
  std::array<uint8_t, 2> io_ports;  ///< I/O ports (Port A and Port B).
  std::array<uint8_t, 2> ddrs;  ///< Data Direction Registers (Port A, Port B).

  // Timer management.
  uint32_t timer_counter = 0;  ///< Internal counter, in CPU cycles.
  int timer_interval = 1;      ///< Divider factor (1, 8, 64, or 1024).

  bool timer_expired = false;  ///< Set when the timer reaches zero.

  /**
   * @brief Load the timer with a new starting value and divider interval.
   * @param value    Initial timer value written by the CPU.
   * @param interval Divider factor to apply (1, 8, 64, or 1024).
   */
  void set_timer(uint8_t value, int interval);

  /**
   * @brief Read the current timer value, as seen by the CPU.
   * @return The current timer count.
   */
  uint8_t get_timer_value();
};
