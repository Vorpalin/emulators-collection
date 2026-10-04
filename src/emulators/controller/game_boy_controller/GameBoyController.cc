#include "emulators/controller/game_boy_controller/GameBoyController.hh"

void GameBoyController::setState(const GameBoyControllerState& state) {
  buttons = state.buttons;
  selectBits = state.selectBits;
}

GameBoyControllerState GameBoyController::getState() const {
  return {buttons, selectBits};
}

void GameBoyController::reset() {
  buttons = 0x00;
  selectBits = 0x30;
}

void GameBoyController::setButton(Button button, bool pressed) {
  bool wasUnset = !(buttons & (1 << button));
  if (pressed)
    buttons |= (1 << button);
  else
    buttons &= ~(1 << button);

  if (!pressed || !wasUnset) return;

  bool isDirection = button <= Down;  // Right, Left, Up, Down = 0-3
  bool selectDirections = !(selectBits & 0x10);
  bool selectActions = !(selectBits & 0x20);
  bool groupSelected = isDirection ? selectDirections : selectActions;

  if (groupSelected) interrupts->request(GameBoyInterruptController::Joypad);
}

uint8_t GameBoyController::read() const {
  uint8_t result = 0xC0 | selectBits;
  bool selectDirections = !(selectBits & 0x10);
  bool selectActions = !(selectBits & 0x20);
  uint8_t lowNibble = 0x0F;
  if (selectDirections) lowNibble &= ~(buttons & 0x0F);
  if (selectActions) lowNibble &= ~((buttons >> 4) & 0x0F);
  return result | lowNibble;
}

void GameBoyController::write(uint8_t value) { selectBits = value & 0x30; }
