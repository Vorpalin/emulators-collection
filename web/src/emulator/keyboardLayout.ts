import { useEffect, useState } from 'react';

export type LayoutMap = ReadonlyMap<string, string>;

/**
 * Represents a keyboard with a layout map.
 * This interface extends the EventTarget and provides a method to get the layout map.
 * @interface KeyboardWithLayout
 * @extends {Partial<EventTarget>}
 */
interface KeyboardWithLayout extends Partial<EventTarget> {
  /**
   * Returns a promise that resolves to the layout map of the keyboard.
   * @returns {Promise<LayoutMap>} A promise that resolves to the layout map.
   */
  getLayoutMap(): Promise<LayoutMap>;
}

const SPECIAL_NAMES: Record<string, string> = {
  ArrowUp: '↑',
  ArrowDown: '↓',
  ArrowLeft: '←',
  ArrowRight: '→',
  Space: 'Space',
  Enter: 'Enter',
  Backspace: 'Backspace',
  Tab: 'Tab',
  ShiftLeft: 'Shift left',
  ShiftRight: 'Shift right',
  ControlLeft: 'Ctrl left',
  ControlRight: 'Ctrl right',
  AltLeft: 'Alt left',
  AltRight: 'Alt right',
};

/**
 * Returns the keyboard object with layout information if available.
 * @returns {KeyboardWithLayout | undefined} The keyboard object or undefined if not available.
 */
function getKeyboard(): KeyboardWithLayout | undefined {
  return (navigator as Navigator & { keyboard?: KeyboardWithLayout }).keyboard;
}

/**
 * A custom React hook that returns the current keyboard layout map, or null if not available.
 * @returns {LayoutMap | null} The current keyboard layout map or null if not available.
 */
export function useLayoutMap(): LayoutMap | null {
  const [layout, setLayout] = useState<LayoutMap | null>(null);

  useEffect(() => {
    const keyboard = getKeyboard();
    if (!keyboard?.getLayoutMap) return;

    let cancelled = false;
    const load = () => {
      keyboard
        .getLayoutMap()
        .then((map) => {
          if (!cancelled) setLayout(map);
        })
        .catch(() => {});
    };

    load();

    keyboard.addEventListener?.('layoutchange', load);
    window.addEventListener('focus', load);

    return () => {
      cancelled = true;
      keyboard.removeEventListener?.('layoutchange', load);
      window.removeEventListener('focus', load);
    };
  }, []);

  return layout;
}

/**
 * Returns a human-readable label for a given keyboard code, using the provided layout map if available.
 * @param code The keyboard code to get the label for.
 * @param layout The layout map to use for localization, or null if not available.
 * @returns The human-readable label for the given keyboard code.
 */
export function keyLabelFor(code: string, layout: LayoutMap | null): string {
  if (SPECIAL_NAMES[code]) return SPECIAL_NAMES[code];

  const printed = layout?.get(code);
  if (printed) return printed.length === 1 ? printed.toUpperCase() : printed;

  if (code.startsWith('Key')) return code.slice(3);
  if (code.startsWith('Digit')) return code.slice(5);
  if (code.startsWith('Numpad')) return `Num ${code.slice(6)}`;
  return code;
}

/**
 * A custom React hook that returns a function to get the human-readable label for a given keyboard code.
 * @returns A function that takes a keyboard code and returns its human-readable label.
 */
export function useKeyLabel(): (code: string) => string {
  const layout = useLayoutMap();
  return (code) => keyLabelFor(code, layout);
}

/**
 * Localizes a keyboard binding code based on the provided layout map.
 * @param code The keyboard binding code to localize.
 * @param layout The layout map to use for localization, or null if not available.
 * @returns The localized keyboard binding code.
 */
export function localizeBinding(code: string, layout: LayoutMap | null): string {
  if (!layout || !code.startsWith('Key')) return code;
  const letter = code.slice(3).toLowerCase();
  for (const [physical, printed] of layout) {
    if (printed === letter) return physical;
  }
  return code;
}

/**
 * Localizes a set of keyboard bindings based on the provided layout map.
 * @param bindings A record of keyboard binding codes to localize.
 * @param layout The layout map to use for localization, or null if not available.
 * @returns A record of the localized keyboard binding codes.
 */
export function localizeBindings(
  bindings: Record<string, string>,
  layout: LayoutMap | null,
): Record<string, string> {
  return Object.fromEntries(
    Object.entries(bindings).map(([id, code]) => [id, localizeBinding(code, layout)]),
  );
}
