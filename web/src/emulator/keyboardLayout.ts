import { useEffect, useState } from 'react';

export type LayoutMap = ReadonlyMap<string, string>;

interface KeyboardWithLayout extends Partial<EventTarget> {
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

function getKeyboard(): KeyboardWithLayout | undefined {
  return (navigator as Navigator & { keyboard?: KeyboardWithLayout }).keyboard;
}

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

export function keyLabelFor(code: string, layout: LayoutMap | null): string {
  if (SPECIAL_NAMES[code]) return SPECIAL_NAMES[code];

  const printed = layout?.get(code);
  if (printed) return printed.length === 1 ? printed.toUpperCase() : printed;

  if (code.startsWith('Key')) return code.slice(3);
  if (code.startsWith('Digit')) return code.slice(5);
  if (code.startsWith('Numpad')) return `Num ${code.slice(6)}`;
  return code;
}

export function useKeyLabel(): (code: string) => string {
  const layout = useLayoutMap();
  return (code) => keyLabelFor(code, layout);
}

export function localizeBinding(code: string, layout: LayoutMap | null): string {
  if (!layout || !code.startsWith('Key')) return code;
  const letter = code.slice(3).toLowerCase();
  for (const [physical, printed] of layout) {
    if (printed === letter) return physical;
  }
  return code;
}

export function localizeBindings(
  bindings: Record<string, string>,
  layout: LayoutMap | null,
): Record<string, string> {
  return Object.fromEntries(
    Object.entries(bindings).map(([id, code]) => [id, localizeBinding(code, layout)]),
  );
}
