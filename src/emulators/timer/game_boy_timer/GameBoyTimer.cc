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
      {"reloadDelay", state.reloadDelay},
  };
}

void from_json(const nlohmann::json& j, GameBoyTimerState& state) {
  j.at("divider").get_to(state.divider);
  j.at("timerCounter").get_to(state.timerCounter);
  j.at("timerModulo").get_to(state.timerModulo);
  j.at("timerControl").get_to(state.timerControl);
  j.at("timerCycles").get_to(state.timerCycles);
  j.at("dividerCounter").get_to(state.dividerCounter);
  state.reloadDelay = j.value("reloadDelay", static_cast<uint8_t>(0));
}

void GameBoyTimer::setState(const GameBoyTimerState& state) {
  divider = state.divider;
  timerCounter = state.timerCounter;
  timerModulo = state.timerModulo;
  timerControl = state.timerControl;
  timerCycles = state.timerCycles;
  dividerCounter = state.dividerCounter;
  reloadDelay = state.reloadDelay;
}

GameBoyTimerState GameBoyTimer::getState() const {
  GameBoyTimerState state;
  state.divider = divider;
  state.timerCounter = timerCounter;
  state.timerModulo = timerModulo;
  state.timerControl = timerControl;
  state.timerCycles = timerCycles;
  state.dividerCounter = dividerCounter;
  state.reloadDelay = reloadDelay;
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
  reloadDelay = 0;
  if (interrupts) interrupts->reset();
}

uint8_t GameBoyTimer::timerInput() const {
  if (!(timerControl & 0x04)) return 0;

  // TAC selects one of four DIV bits. TIMA increments on a falling edge of
  // this signal, not after an independently accumulated period.
  constexpr uint8_t selectedBits[] = {9, 3, 5, 7};
  return static_cast<uint8_t>((divider >> selectedBits[timerControl & 0x03]) &
                              0x01);
}

void GameBoyTimer::incrementTIMA() {
  // During the overflow/reload window further timer edges are ignored.
  if (reloadDelay != 0) return;

  ++timerCounter;
  if (timerCounter == 0) reloadDelay = 4;
}

void GameBoyTimer::tick(uint8_t cycles) {
  for (uint16_t i = 0; i < cycles; ++i) {
    // TIMA reloads four T-cycles after overflowing. The interrupt is raised
    // on the reload cycle, rather than immediately on the overflow edge.
    if (reloadDelay > 0) {
      --reloadDelay;
      if (reloadDelay == 0) {
        timerCounter = timerModulo;
        if (interrupts) interrupts->request(GameBoyInterruptController::Timer);
      }
    }

    const uint8_t oldInput = timerInput();
    ++divider;
    const uint8_t newInput = timerInput();
    if (oldInput && !newInput) incrementTIMA();
  }
}

uint8_t GameBoyTimer::read(uint16_t address) {
  switch (address) {
    case 0xFF04:
      return static_cast<uint8_t>(divider >> 8);
    case 0xFF05:
      return timerCounter;
    case 0xFF06:
      return timerModulo;
    case 0xFF07:
      return static_cast<uint8_t>(0xF8 | timerControl);
    default:
      return 0xFF;
  }
}

void GameBoyTimer::write(uint16_t address, uint8_t value) {
  switch (address) {
    case 0xFF04: {
      const uint8_t oldInput = timerInput();
      divider = 0;
      if (oldInput && !timerInput()) incrementTIMA();
      break;
    }
    case 0xFF05:
      timerCounter = value;
      // Writing TIMA during the overflow delay cancels the pending reload
      // and its interrupt.
      if (reloadDelay > 0) reloadDelay = 0;
      break;
    case 0xFF06:
      timerModulo = value;
      break;
    case 0xFF07: {
      const uint8_t oldInput = timerInput();
      timerControl = value & 0x07;
      if (oldInput && !timerInput()) incrementTIMA();
      break;
    }
    default:
      break;
  }
}
