export type SystemId = 'chip8' | 'atari2600' | 'gameboy';

/**
 * Defines the structure of an action within a system, including its unique identifier, display label, and associated key code.
 * Each action represents a specific input or control that can be triggered within the emulator session for the corresponding system.
 * @property {string} id - The unique identifier for the action, used to reference it in bindings and input handling.
 * @property {string} label - The display label for the action, used in user interfaces to describe the action.
 * @property {number} key - The key code associated with the action, representing the input that triggers it.
 */
export interface ActionDef {
  /**
   * The unique identifier for the action, used to reference it in bindings and input handling.
   */
  id: string;
  /**
   * The display label for the action, used in user interfaces to describe the action.
   */
  label: string;
  /**
   * The key code associated with the action, representing the input that triggers it.
   */
  key: number;
}

/**
 * Defines the structure of a system within the emulator, including its unique identifier, display label, supported file extensions, display aspect ratio, available actions, and default input bindings.
 * Each system represents a specific platform or console that can be emulated, with its own set of actions and input configurations.
 * @property {SystemId} id - The unique identifier for the system, used to reference it in the emulator session.
 * @property {string} label - The display label for the system, used in user interfaces to describe the system.
 * @property {string[]} extensions - The list of supported file extensions for ROMs associated with the system.
 * @property {number} displayAspect - The aspect ratio of the display for the system, used to correctly render the output.
 * @property {ActionDef[]} actions - The list of available actions for the system, defining the inputs that can be triggered.
 * @property {Record<string, string>} defaultBindings - The default input bindings for the system, mapping action identifiers to key codes or input sources.
 */
export interface SystemDef {
  /**
   * The unique identifier for the system, used to reference it in the emulator session.
   */
  id: SystemId;
  /**
   * The display label for the system, used in user interfaces to describe the system.
   */
  label: string;
  /**
   * The list of supported file extensions for ROMs associated with the system.
   */
  extensions: string[];
  /**
   * The aspect ratio of the display for the system, used to correctly render the output.
   */
  displayAspect: number;
  /**
   * The list of available actions for the system, defining the inputs that can be triggered.
   */
  actions: ActionDef[];
  /**
   * The default input bindings for the system, mapping action identifiers to key codes or input sources.
   */
  defaultBindings: Record<string, string>;
}

const HEX = '0123456789ABCDEF'.split('');

export const SYSTEMS: Record<SystemId, SystemDef> = {
  chip8: {
    id: 'chip8',
    label: 'CHIP-8',
    extensions: ['.ch8'],
    displayAspect: 2,
    actions: HEX.map((h) => ({ id: h, label: h, key: parseInt(h, 16) })),
    defaultBindings: Object.fromEntries(
      HEX.map((h) => [h, /\d/.test(h) ? `Digit${h}` : `Key${h}`]),
    ),
  },
  atari2600: {
    id: 'atari2600',
    label: 'Atari 2600',
    extensions: ['.a26'],
    displayAspect: 320 / 312,
    actions: [
      { id: 'RIGHT', label: 'Right', key: 0 },
      { id: 'LEFT', label: 'Left', key: 1 },
      { id: 'DOWN', label: 'Down', key: 2 },
      { id: 'UP', label: 'Up', key: 3 },
      { id: 'FIRE', label: 'Fire', key: 4 },
      { id: 'RESET', label: 'Reset', key: 5 },
      { id: 'SELECT', label: 'Select', key: 6 },
    ],
    defaultBindings: {
      RIGHT: 'ArrowRight',
      LEFT: 'ArrowLeft',
      DOWN: 'ArrowDown',
      UP: 'ArrowUp',
      FIRE: 'Space',
      RESET: 'Enter',
      SELECT: 'ShiftRight',
    },
  },
  gameboy: {
    id: 'gameboy',
    label: 'Game Boy',
    extensions: ['.gb'],
    displayAspect: 160 / 144,
    actions: [
      { id: 'RIGHT', label: 'Right', key: 0 },
      { id: 'LEFT', label: 'Left', key: 1 },
      { id: 'UP', label: 'Up', key: 2 },
      { id: 'DOWN', label: 'Down', key: 3 },
      { id: 'A', label: 'A', key: 4 },
      { id: 'B', label: 'B', key: 5 },
      { id: 'SELECT', label: 'Select', key: 6 },
      { id: 'START', label: 'Start', key: 7 },
    ],
    defaultBindings: {
      RIGHT: 'ArrowRight',
      LEFT: 'ArrowLeft',
      UP: 'ArrowUp',
      DOWN: 'ArrowDown',
      A: 'KeyA',
      B: 'KeyB',
      SELECT: 'ShiftRight',
      START: 'Enter',
    },
  },
};

export const SYSTEM_LIST: SystemDef[] = Object.values(SYSTEMS);

/**
 * Detects the system type based on the provided filename by checking its extension against the known extensions for each system.
 * If a matching system is found, its SystemId is returned; otherwise, null is returned.
 * @param filename - The name of the file (including its extension) to be checked against the known system extensions.
 * @returns The SystemId of the detected system if a match is found; otherwise, null.
 */
export function detectSystem(filename: string): SystemId | null {
  const lower = filename.toLowerCase();
  const found = SYSTEM_LIST.find((s) => s.extensions.some((ext) => lower.endsWith(ext)));
  return found ? found.id : null;
}

export const ACCEPTED_EXTENSIONS = SYSTEM_LIST.flatMap((s) => s.extensions).join(',');
