#include "emulators/interrupt_controller/game_boy_interrupt_controller/GameBoyInterruptController.hh"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

void GameBoyInterruptController::setState(
    const GameBoyInterruptControllerState& state) {
  ifReg = state.ifReg;
  ieReg = state.ieReg;
}

GameBoyInterruptControllerState GameBoyInterruptController::getState() const {
  return {
      .ifReg = ifReg,
      .ieReg = ieReg,
  };
}

void to_json(nlohmann::json& j, const GameBoyInterruptControllerState& state) {
  j = {
      {"ifReg", state.ifReg},
      {"ieReg", state.ieReg},
  };
}

void from_json(const nlohmann::json& j,
               GameBoyInterruptControllerState& state) {
  j.at("ifReg").get_to(state.ifReg);
  j.at("ieReg").get_to(state.ieReg);
}

void GameBoyInterruptController::reset() {
  ifReg = 0x00;
  ieReg = 0x00;
}

void GameBoyInterruptController::request(Flag flag) { ifReg |= flag; }

uint8_t GameBoyInterruptController::readIF() const { return 0xE0 | ifReg; }
void GameBoyInterruptController::writeIF(uint8_t value) {
  ifReg = value & 0x1F;
}

uint8_t GameBoyInterruptController::readIE() const { return ieReg; }
void GameBoyInterruptController::writeIE(uint8_t value) { ieReg = value; }

uint8_t GameBoyInterruptController::pending() const {
  return ifReg & ieReg & 0x1F;
}
