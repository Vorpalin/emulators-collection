#pragma once

#include <cstdint>

#include "emulators/bus/Bus.hh"

/**
 * @file Z80Bus.hh
 * @brief Bus interface required by the Zilog Z80 core.
 */

/**
 * @class Z80Bus
 * @brief A Bus that also exposes the Z80's separate 16-bit I/O port space.
 *
 * The Z80 has two address spaces: memory (accessed through Bus::read and
 * Bus::write) and I/O ports (IN/OUT instructions). On a real Z80 the low byte
 * of the port address comes from the instruction operand or register C, and
 * the high byte from A or B depending on the instruction, so the full 16-bit
 * address is passed to the handlers. Many systems (e.g. the Sega Master
 * System) only decode the low 8 bits.
 */
class Z80Bus : public Bus {
 public:
  /**
   * @brief Read a byte from an I/O port.
   * @param port Full 16-bit port address as presented on the address bus.
   * @return The byte read.
   */
  virtual uint8_t readPort(uint16_t port) = 0;

  /**
   * @brief Write a byte to an I/O port.
   * @param port  Full 16-bit port address as presented on the address bus.
   * @param value Byte to write.
   */
  virtual void writePort(uint16_t port, uint8_t value) = 0;
};
