#pragma once

#include <cstdint>

#include "emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.hh"

/**
 * @file GameBoyController.hh
 * @brief Game Boy joypad emulation (register P1/JOYP at 0xFF00).
 */

/**
 * @class GameBoyController
 * @brief Emulates the joypad register at 0xFF00.
 *
 * On real hardware the buttons are active-low and arranged in a 2x4 matrix
 * selected through bits 4 and 5 of the register. This class exposes a clean
 * active-high API (`setButton(Right, true)` means "pressed") and performs the
 * inversion internally when the register is read.
 *
 * A Joypad interrupt is requested on the falling edge of a button (i.e. when
 * it goes from released to pressed) if its group is currently selected.
 */
class GameBoyController {
 public:
  /**
   * @brief Logical buttons. The value is the bit index in the internal
   *        `buttons` state (bits 0-3: D-pad, bits 4-7: action buttons).
   */
  enum Button : uint8_t {
    Right = 0,   ///< D-pad right
    Left = 1,    ///< D-pad left
    Up = 2,      ///< D-pad up
    Down = 3,    ///< D-pad down
    A = 4,       ///< A button
    B = 5,       ///< B button
    Select = 6,  ///< Select button
    Start = 7,   ///< Start button
  };

  /**
   * @brief Constructs the controller.
   * @param interrupts Interrupt controller used to raise the Joypad
   *        interrupt. Not owned; must outlive this object.
   */
  explicit GameBoyController(GameBoyInterruptController* interrupts)
      : interrupts(interrupts) {}

  /** @brief Releases all buttons and deselects both button groups. */
  void reset();

  /**
   * @brief Presses or releases a button.
   *
   * If the button transitions from released to pressed and the group it
   * belongs to (D-pad or actions) is currently selected through 0xFF00,
   * a Joypad interrupt is requested.
   *
   * @param button  The button to update.
   * @param pressed `true` if pressed, `false` if released.
   */
  void setButton(Button button, bool pressed);

  /**
   * @brief Reads the joypad register (0xFF00).
   *
   * Bits 6-7 read as 1, bits 4-5 return the current select lines, and
   * bits 0-3 are active-low button states for the selected group(s).
   *
   * @return Value of the register.
   */
  uint8_t read() const;

  /**
   * @brief Writes the joypad register (0xFF00).
   *
   * Only bits 4 (select D-pad) and 5 (select actions) are writable; both
   * are active-low.
   *
   * @param value Value written by the CPU.
   */
  void write(uint8_t value);

 private:
  GameBoyInterruptController*
      interrupts;  ///< Interrupt controller (not owned).
  /// Pressed-button bitmask: bit set = pressed; bits 0-3 D-pad, 4-7
  /// A/B/Select/Start.
  uint8_t buttons = 0x00;
  /// Bits 4-5 of 0xFF00 (active-low select lines).
  uint8_t selectBits = 0x30;
};
