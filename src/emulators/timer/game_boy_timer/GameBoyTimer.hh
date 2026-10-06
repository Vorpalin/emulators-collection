#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

#include "emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.hh"

/**
 * @struct GameBoyTimerState
 * @brief Serializable state of the Game Boy Timer.
 * @details This structure holds the state of the Game Boy Timer for the purpose
 * of saving and loading the Game Boy Timer's state.
 */
struct GameBoyTimerState {
  uint16_t divider;         // Divider register (0xFF04)
  uint8_t timerCounter;     // Timer counter (0xFF05)
  uint8_t timerModulo;      // Timer modulo (0xFF06)
  uint8_t timerControl;     // Timer control (0xFF07)
  uint16_t timerCycles;     // Internal counter for timer ticks
  uint32_t dividerCounter;  // Internal counter for divider ticks
};

/**
 * @brief Serializes a GameBoyTimerState to JSON.
 * @param j The JSON object to serialize into.
 * @param state The GameBoyTimerState to serialize.
 */
void to_json(nlohmann::json& j, const GameBoyTimerState& state);
/**
 * @brief Deserializes a GameBoyTimerState from JSON.
 * @param j The JSON object to deserialize from.
 * @param state The GameBoyTimerState to populate.
 */
void from_json(const nlohmann::json& j, GameBoyTimerState& state);

/**
 * @class GameBoyTimer
 * @brief Emulates the Game Boy Timer.
 */
class GameBoyTimer {
 public:
  /**
   * @brief Construct a GameBoyTimer and reset it to its power-up state.
   * @param interrupts Pointer to the interrupt controller.
   */
  explicit GameBoyTimer(GameBoyInterruptController* interrupts)
      : interrupts(interrupts) {
    divider = 0;
    timerCounter = 0;
    timerModulo = 0;
    timerControl = 0;
    timerCycles = 0;
    dividerCounter = 0;
  }
  GameBoyTimer();

  /**
   * @brief Reset the Game Boy Timer to its power-up state.
   */
  void reset();

  /**
   * @brief Advance the timer by the given number of CPU cycles.
   * @param cycles Number of CPU cycles to advance the timer by.
   */
  void tick(uint8_t cycles);

  /**
   * @brief Read from a Game Boy Timer register.
   * @param address The register address to read from.
   * @return The value read.
   */
  uint8_t read(uint16_t address);

  /**
   * @brief Write to a Game Boy Timer register.
   * @param address The register address to write to.
   * @param value The value to write.
   */
  void write(uint16_t address, uint8_t value);

  /**
   * @brief Set the state of the Game Boy Timer from a serializable structure.
   * @param state The state to set.
   */
  void setState(const GameBoyTimerState& state);
  /**
   * @brief Get the current Game Boy Timer state as a serializable structure.
   * @return The current Game Boy Timer state.
   */
  GameBoyTimerState getState() const;

 private:
  /**
   * @brief Calculate the timer threshold based on the current timer control.
   * @return The timer threshold.
   */
  uint32_t timerThreshold() const;

  uint16_t divider;         ///< Divider register (0xFF04)
  uint8_t timerCounter;     ///< Timer counter (0xFF05)
  uint8_t timerModulo;      ///< Timer modulo (0xFF06)
  uint8_t timerControl;     ///< Timer control (0xFF07)
  uint16_t timerCycles;     ///< Internal counter for timer ticks
  uint32_t dividerCounter;  ///< Internal counter for divider ticks
  GameBoyInterruptController* interrupts;  ///< Handles timer interrupts
};
