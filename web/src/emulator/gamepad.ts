import { useEffect, useState } from 'react';
import type { SystemId } from './systems';

type Direction = 'up' | 'down' | 'left' | 'right';

/**
 * Represents the mapping of a gamepad for a specific system.
 * @property directions - A record mapping each direction to its corresponding action string.
 * @property buttons - A record mapping each button action to an array of button indices.
 */
export interface PadMapping {
  /**
   * A record mapping each direction to its corresponding action string.
   */
  directions: Record<Direction, string>;
  /**
   * A record mapping each button action to an array of button indices.
   */
  buttons: Record<string, number[]>;
}

const FACE_BUTTONS = [0, 1, 2, 3];
const STICK_DEADZONE = 0.5;

const GB_LIKE_DIRECTIONS: Record<Direction, string> = {
  up: 'UP',
  down: 'DOWN',
  left: 'LEFT',
  right: 'RIGHT',
};

/**
 * A record that defines the gamepad mappings for different systems.
 * Each system has its own mapping of directions and buttons.
 * @type {Record<SystemId, PadMapping>}
 */
export const GAMEPAD_MAPPINGS: Record<SystemId, PadMapping> = {
  gameboy: {
    directions: GB_LIKE_DIRECTIONS,
    buttons: {
      A: [1, 3],
      B: [0, 2], // bouton du bas et de gauche
      SELECT: [8],
      START: [9],
    },
  },
  atari2600: {
    directions: GB_LIKE_DIRECTIONS,
    buttons: {
      FIRE: FACE_BUTTONS,
      SELECT: [8],
      RESET: [9],
    },
  },
  chip8: {
    directions: { up: '2', down: '8', left: '4', right: '6' },
    buttons: {
      '5': FACE_BUTTONS,
    },
  },
};

/**
 * Reads the actions from a gamepad based on its mapping.
 * @param mapping The gamepad mapping to use.
 * @returns A set of strings representing the currently pressed actions.
 */
export function readGamepadActions(mapping: PadMapping): Set<string> {
  const pressed = new Set<string>();
  if (typeof navigator.getGamepads !== 'function') return pressed;

  for (const pad of navigator.getGamepads()) {
    if (!pad || !pad.connected) continue;

    const down = (index: number) => pad.buttons[index]?.pressed === true;
    const x = pad.axes[0] ?? 0;
    const y = pad.axes[1] ?? 0;

    if (down(12) || y < -STICK_DEADZONE) pressed.add(mapping.directions.up);
    if (down(13) || y > STICK_DEADZONE) pressed.add(mapping.directions.down);
    if (down(14) || x < -STICK_DEADZONE) pressed.add(mapping.directions.left);
    if (down(15) || x > STICK_DEADZONE) pressed.add(mapping.directions.right);

    for (const [actionId, indices] of Object.entries(mapping.buttons)) {
      if (indices.some((i) => down(i))) pressed.add(actionId);
    }
  }

  return pressed;
}

/**
 * A custom React hook that returns the name of the first connected gamepad, or null if no gamepad is connected.
 * @returns The name of the first connected gamepad, or null if no gamepad is connected.
 */
export function useGamepadName(): string | null {
  const [name, setName] = useState<string | null>(null);

  useEffect(() => {
    const refresh = () => {
      const pads = typeof navigator.getGamepads === 'function' ? navigator.getGamepads() : [];
      const pad = Array.from(pads).find((p) => p?.connected);
      setName(pad ? pad.id : null);
    };

    refresh();
    window.addEventListener('gamepadconnected', refresh);
    window.addEventListener('gamepaddisconnected', refresh);
    return () => {
      window.removeEventListener('gamepadconnected', refresh);
      window.removeEventListener('gamepaddisconnected', refresh);
    };
  }, []);

  return name;
}
