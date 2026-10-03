export type SystemId = 'chip8' | 'atari2600' | 'gameboy';

export interface ActionDef {
  id: string;
  label: string;
  key: number;
}

export interface SystemDef {
  id: SystemId;
  label: string;
  extensions: string[];
  displayAspect: number;
  actions: ActionDef[];
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

export function detectSystem(filename: string): SystemId | null {
  const lower = filename.toLowerCase();
  const found = SYSTEM_LIST.find((s) => s.extensions.some((ext) => lower.endsWith(ext)));
  return found ? found.id : null;
}

export const ACCEPTED_EXTENSIONS = SYSTEM_LIST.flatMap((s) => s.extensions).join(',');
