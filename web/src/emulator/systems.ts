/**
 * Description des consoles supportées. L'identifiant (`id`) est aussi la
 * chaîne passée à `emulator.load(type, rom)` côté C++ (bindings.cc) et la
 * valeur de la colonne `games.system` dans Supabase.
 *
 * Les `key` correspondent aux indices attendus par `Console::setKey()` :
 *  - chip8     : 0x0 à 0xF
 *  - atari2600 : Atari2600::Key  (0 droite, 1 gauche, 2 bas, 3 haut, 4 tir, 5 reset, 6 select)
 *  - gameboy   : GameBoy::Key    (0 droite, 1 gauche, 2 haut, 3 bas, 4 A, 5 B, 6 select, 7 start)
 */
export type SystemId = 'chip8' | 'atari2600' | 'gameboy';

export interface ActionDef {
  id: string; // identifiant stable (clé dans key_bindings)
  label: string; // libellé affiché
  key: number; // indice envoyé au cœur C++
}

export interface SystemDef {
  id: SystemId;
  label: string;
  extensions: string[]; // en minuscules, avec le point
  displayAspect: number; // rapport largeur/hauteur à l'écran (les pixels de l'Atari sont 2x plus larges)
  actions: ActionDef[];
  defaultBindings: Record<string, string>; // action.id -> KeyboardEvent.code
}

const HEX = '0123456789ABCDEF'.split('');

export const SYSTEMS: Record<SystemId, SystemDef> = {
  chip8: {
    id: 'chip8',
    label: 'CHIP-8',
    extensions: ['.ch8'],
    displayAspect: 2, // 64x32 (et 128x64 en Super-CHIP)
    actions: HEX.map((h) => ({ id: h, label: h, key: parseInt(h, 16) })),
    defaultBindings: Object.fromEntries(
      HEX.map((h) => [h, /\d/.test(h) ? `Digit${h}` : `Key${h}`]),
    ),
  },
  atari2600: {
    id: 'atari2600',
    label: 'Atari 2600',
    extensions: ['.a26'],
    displayAspect: 320 / 312, // 160x312, pixels doublés en largeur
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

/** Devine la console à partir de l'extension du fichier (null si inconnue). */
export function detectSystem(filename: string): SystemId | null {
  const lower = filename.toLowerCase();
  const found = SYSTEM_LIST.find((s) => s.extensions.some((ext) => lower.endsWith(ext)));
  return found ? found.id : null;
}

/** Toutes les extensions acceptées, pour l'attribut `accept` d'un <input>. */
export const ACCEPTED_EXTENSIONS = SYSTEM_LIST.flatMap((s) => s.extensions).join(',');
