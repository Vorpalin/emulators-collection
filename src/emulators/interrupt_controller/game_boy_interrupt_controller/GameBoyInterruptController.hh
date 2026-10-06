#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

/**
 * @file GameBoyInterruptController.hh
 * @brief Game Boy interrupt controller emulation.
 * @details This file contains the declaration of the GameBoyInterruptController
 * class, which emulates the interrupt controller of the Game Boy.
 */

/**
 * @struct GameBoyInterruptControllerState
 * @brief Serializable state of the Game Boy interrupt controller.
 * @details This structure holds the state of the interrupt controller for the
 * purpose of saving and loading the controller's state.
 */
struct GameBoyInterruptControllerState {
  uint8_t ifReg;
  uint8_t ieReg;
};

/**
 * @brief Serializes a GameBoyInterruptControllerState to JSON.
 * @param j The JSON object to serialize into.
 * @param state The GameBoyInterruptControllerState to serialize.
 */
void to_json(nlohmann::json& j, const GameBoyInterruptControllerState& state);

/**
 * @brief Deserializes a GameBoyInterruptControllerState from JSON.
 * @param j The JSON object to deserialize from.
 * @param state The GameBoyInterruptControllerState to populate.
 */
void from_json(const nlohmann::json& j, GameBoyInterruptControllerState& state);

class GameBoyInterruptController {
 public:
  /**
   * @brief Interrupt flags.
   * @details These are the possible interrupt flags that can be requested.
   */
  enum Flag : uint8_t {
    VBlank = 1 << 0,
    LCDStat = 1 << 1,
    Timer = 1 << 2,
    Serial = 1 << 3,
    Joypad = 1 << 4,
  };

  /**
   * @brief Resets the interrupt controller to its initial state.
   */
  void reset();

  /**
   * @brief Requests an interrupt by setting the corresponding flag in the IF
   * register.
   * @param flag The interrupt flag to request.
   */
  void request(Flag flag);

  /**
   * @brief Reads the IF register.
   * @return The value of the IF register.
   * @note The top 3 bits of the IF register are always high.
   */
  uint8_t readIF() const;

  /**
   * @brief Writes to the IF register.
   * @param value The value to write to the IF register.
   * @note Only the lower 5 bits of the IF register are writable; the top 3 bits
   * are always high.
   */
  void writeIF(uint8_t value);

  /**
   * @brief Reads the IE register.
   * @return The value of the IE register.
   */
  uint8_t readIE() const;
  /**
   * @brief Writes to the IE register.
   * @param value The value to write to the IE register.
   * @note Only the lower 5 bits of the IE register are writable; the top 3 bits
   * are always high.
   */
  void writeIE(uint8_t value);

  // Interrupts that are both requested and enabled; the CPU services the
  // lowest-numbered bit set here and clears it via writeIF.
  uint8_t pending() const;

  /**
   * @brief Sets the interrupt controller state from a serializable structure.
   * @param state The state to set.
   */
  void setState(const GameBoyInterruptControllerState& state);
  /**
   * @brief Gets the current interrupt controller state as a serializable
   * structure.
   * @return The current interrupt controller state.
   */
  GameBoyInterruptControllerState getState() const;

 private:
  uint8_t ifReg = 0x00;
  uint8_t ieReg = 0x00;
};
