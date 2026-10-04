#include "emulators/timer/game_boy_timer/GameBoyTimer.hh"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void to_json(nlohmann::json& j, const GameBoyTimerState& state) {
  j = {
      {"divider", state.divider},
      {"timerCounter", state.timerCounter},
      {"timerModulo", state.timerModulo},
      {"timerControl", state.timerControl},
      {"timerCycles", state.timerCycles},
      {"dividerCounter", state.dividerCounter},
  };
}

void from_json(const nlohmann::json& j, GameBoyTimerState& state) {
  j.at("divider").get_to(state.divider);
  j.at("timerCounter").get_to(state.timerCounter);
  j.at("timerModulo").get_to(state.timerModulo);
  j.at("timerControl").get_to(state.timerControl);
  j.at("timerCycles").get_to(state.timerCycles);
  j.at("dividerCounter").get_to(state.dividerCounter);
}

void GameBoyTimer::setState(const GameBoyTimerState& state) {
  divider = state.divider;
  timerCounter = state.timerCounter;
  timerModulo = state.timerModulo;
  timerControl = state.timerControl;
  timerCycles = state.timerCycles;
  dividerCounter = state.dividerCounter;
}

GameBoyTimerState GameBoyTimer::getState() const {
  GameBoyTimerState state;
  state.divider = divider;
  state.timerCounter = timerCounter;
  state.timerModulo = timerModulo;
  state.timerControl = timerControl;
  state.timerCycles = timerCycles;
  state.dividerCounter = dividerCounter;
  return state;
}

GameBoyTimer::GameBoyTimer() {
  interrupts = nullptr;
  reset();
}

void GameBoyTimer::reset() {
  divider = 0;
  timerCounter = 0;
  timerModulo = 0;
  timerControl = 0;
  timerCycles = 0;
  dividerCounter = 0;
  if (interrupts) interrupts->reset();
}

void GameBoyTimer::tick(uint8_t cycles) {
  // Update the divider register (0xFF04)
  divider += cycles;
  while (divider >= 256) {
    divider -= 256;
    divider =
        (divider + 1) & 0xFF;  // Increment the divider and wrap around at 256
  }

  // Check if the timer is enabled
  if (!(timerControl & 0x04)) {
    return;  // Timer is disabled, do not update the timer counter
  }
  timerCycles += cycles;

  timerCounter += cycles;

  // Determine the timer frequency based on the control register
  uint16_t timerFrequency = timerThreshold();

  // Increment the timer counter based on the frequency
  while (timerCycles >= timerFrequency) {
    timerCycles -= timerFrequency;
    timerCounter++;
    if (timerCounter == 0) {       // Overflow occurred
      timerCounter = timerModulo;  // Reset to modulo value
      interrupts->request(
          GameBoyInterruptController::Timer);  // Request timer interrupt
    }
  }
}

uint8_t GameBoyTimer::read(uint16_t address) {
  switch (address) {
    case 0xFF04:
      return static_cast<uint8_t>(divider & 0xFF);  // Divider register
    case 0xFF05:
      return timerCounter;  // Timer counter
    case 0xFF06:
      return timerModulo;  // Timer modulo
    case 0xFF07:
      return 0xF8 | timerControl;  // Timer control
    default:
      return 0xFF;  // Invalid read
  }
}

void GameBoyTimer::write(uint16_t address, uint8_t value) {
  switch (address) {
    case 0xFF04:  // Writing to the divider register resets it
      divider = 0;
      dividerCounter = 0;
      break;
    case 0xFF05:  // Timer counter
      timerCounter = value;
      break;
    case 0xFF06:  // Timer modulo
      timerModulo = value;
      break;
    case 0xFF07:                    // Timer control
      timerControl = value & 0x07;  // Only the lower 3 bits are used
      break;
    default:
      // Invalid write, do nothing
      break;
  }
}

uint32_t GameBoyTimer::timerThreshold() const {
  switch (timerControl & 0x03) {
    case 0:
      return 1024;  // 4096 Hz
    case 1:
      return 16;  // 262144 Hz
    case 2:
      return 64;  // 65536 Hz
    case 3:
      return 256;  // 16384 Hz
    default:
      return 1024;  // Default to 4096 Hz
  }
}
