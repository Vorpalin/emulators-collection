#pragma once

#include "emulators/console/master_system/MasterSystem.hh"

/**
 * @file GameGear.hh
 * @brief Sega Game Gear console.
 */

/**
 * @class GameGear
 * @brief A MasterSystem running in Game Gear mode: 160x144 viewport cut out
 *        of the 256x192 VDP picture, 12-bit palette, Start button on port 0
 *        and the PSG stereo register.
 *
 * Same key layout as MasterSystem; Start is the real Start button.
 */
class GameGear : public MasterSystem {
 public:
  static constexpr int kWidth = 160;
  static constexpr int kHeight = 144;

  GameGear() : MasterSystem(SegaVDP::Model::GameGear) {}
};
