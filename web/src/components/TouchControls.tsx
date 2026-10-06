import {
  useCallback,
  useEffect,
  useRef,
  useState,
  type CSSProperties,
  type PointerEvent,
  type ReactNode,
} from 'react';
import type { SystemDef } from '../emulator/systems';

/**
 * Custom hook to determine if the current device matches a given media query.
 * @param query - The media query string to evaluate.
 * @returns A boolean indicating whether the media query matches the current device.
 */
export function useMediaQuery(query: string): boolean {
  const [matches, setMatches] = useState(
    () => typeof window !== 'undefined' && window.matchMedia(query).matches,
  );

  useEffect(() => {
    const mql = window.matchMedia(query);
    const onChange = () => setMatches(mql.matches);
    onChange();
    mql.addEventListener('change', onChange);
    return () => mql.removeEventListener('change', onChange);
  }, [query]);

  return matches;
}

/**
 * Custom hook to determine if the current device is a touch device.
 * @returns A boolean indicating whether the current device is a touch device.
 */
export const useIsTouchDevice = () => useMediaQuery('(pointer: coarse)');

/**
 * SetAction type defines a function that sets the action state for a given control ID.
 * @param id - The ID of the control to set the action for.
 * @param pressed - A boolean indicating whether the control is pressed or released.
 */
type SetAction = (id: string, pressed: boolean) => void;

/**
 * Held type defines a read-only set of strings representing the currently held control IDs.
 */
type Held = ReadonlySet<string>;

const noGestures: CSSProperties = {
  touchAction: 'none',
  userSelect: 'none',
  WebkitUserSelect: 'none',
  WebkitTouchCallout: 'none',
};

const idle = 'bg-slate-700 text-slate-200';
const active = 'bg-cyan-400 text-slate-950';

/**
 * PadButton component renders a button for a gamepad control.
 * @param id - The ID of the control.
 * @param label - The label to display on the button.
 * @param held - A set of currently held control IDs.
 * @param setAction - A function to set the action state for the control.
 * @param className - Additional CSS classes to apply to the button.
 * @returns The rendered PadButton component.
 */
function PadButton({
  id,
  label,
  held,
  setAction,
  className = '',
}: {
  id: string;
  label: string;
  held: Held;
  setAction: SetAction;
  className?: string;
}) {
  const down = (e: PointerEvent<HTMLButtonElement>) => {
    e.preventDefault();
    try {
      e.currentTarget.setPointerCapture(e.pointerId);
    } catch {}
    setAction(id, true);
  };
  const up = () => setAction(id, false);

  return (
    <button
      type="button"
      onPointerDown={down}
      onPointerUp={up}
      onPointerCancel={up}
      onLostPointerCapture={up}
      onContextMenu={(e) => e.preventDefault()}
      style={noGestures}
      className={`font-bold transition-colors ${held.has(id) ? active : idle} ${className}`}
    >
      {label}
    </button>
  );
}

const DIRECTIONS = ['UP', 'DOWN', 'LEFT', 'RIGHT'] as const;

/**
 * DPad component renders a directional pad for game controls.
 * @param held - A set of currently held control IDs.
 * @param setAction - A function to set the action state for the control.
 * @returns The rendered DPad component.
 */
function DPad({ held, setAction }: { held: Held; setAction: SetAction }) {
  const ref = useRef<HTMLDivElement>(null);
  const pointer = useRef<number | null>(null);
  const DEAD_ZONE = 0.28;

  const apply = (x: number, y: number) => {
    const el = ref.current;
    if (!el) return;
    const r = el.getBoundingClientRect();
    const dx = (x - (r.left + r.width / 2)) / (r.width / 2);
    const dy = (y - (r.top + r.height / 2)) / (r.height / 2);
    setAction('LEFT', dx < -DEAD_ZONE);
    setAction('RIGHT', dx > DEAD_ZONE);
    setAction('UP', dy < -DEAD_ZONE);
    setAction('DOWN', dy > DEAD_ZONE);
  };

  const release = () => {
    pointer.current = null;
    for (const id of DIRECTIONS) setAction(id, false);
  };

  const arm = (id: string, position: string) => (
    <div
      className={`absolute pointer-events-none transition-colors ${position} ${
        held.has(id) ? 'bg-cyan-400' : 'bg-slate-700'
      }`}
    />
  );

  return (
    <div
      ref={ref}
      className="relative w-36 h-36"
      style={noGestures}
      onContextMenu={(e) => e.preventDefault()}
      onPointerDown={(e) => {
        if (pointer.current !== null) return;
        e.preventDefault();
        pointer.current = e.pointerId;
        try {
          e.currentTarget.setPointerCapture(e.pointerId);
        } catch {}
        apply(e.clientX, e.clientY);
      }}
      onPointerMove={(e) => {
        if (e.pointerId === pointer.current) apply(e.clientX, e.clientY);
      }}
      onPointerUp={(e) => {
        if (e.pointerId === pointer.current) release();
      }}
      onPointerCancel={(e) => {
        if (e.pointerId === pointer.current) release();
      }}
      onLostPointerCapture={(e) => {
        if (e.pointerId === pointer.current) release();
      }}
    >
      {arm('UP', 'left-1/3 top-0 w-1/3 h-[38%] rounded-t-xl')}
      {arm('DOWN', 'left-1/3 bottom-0 w-1/3 h-[38%] rounded-b-xl')}
      {arm('LEFT', 'top-1/3 left-0 h-1/3 w-[38%] rounded-l-xl')}
      {arm('RIGHT', 'top-1/3 right-0 h-1/3 w-[38%] rounded-r-xl')}
      <div className="absolute pointer-events-none left-1/3 top-1/3 w-1/3 h-1/3 bg-slate-700" />
    </div>
  );
}

/**
 * Pills component renders a set of pill-shaped buttons.
 * @param ids - An array of objects defining the IDs and labels for each pill.
 * @param held - A set of currently held control IDs.
 * @param setAction - A function to set the action state for the control.
 * @returns The rendered Pills component.
 */
function Pills({
  ids,
  held,
  setAction,
}: {
  ids: { id: string; label: string }[];
  held: Held;
  setAction: SetAction;
}) {
  return (
    <div className="flex gap-3 justify-center">
      {ids.map((p) => (
        <PadButton
          key={p.id}
          id={p.id}
          label={p.label.toUpperCase()}
          held={held}
          setAction={setAction}
          className="px-4 py-2 rounded-full text-[11px] tracking-wider"
        />
      ))}
    </div>
  );
}

/**
 * GameBoyButtons component renders the buttons for a Game Boy-style controller.
 * @param props - The properties for the GameBoyButtons component.
 * @returns The rendered GameBoyButtons component.
 */
function GameBoyButtons(props: { held: Held; setAction: SetAction }) {
  return (
    <div className="space-y-4">
      <div className="relative w-32 h-24 mx-auto">
        <PadButton
          id="B"
          label="B"
          {...props}
          className="absolute left-0 bottom-0 w-14 h-14 rounded-full text-lg"
        />
        <PadButton
          id="A"
          label="A"
          {...props}
          className="absolute right-0 top-0 w-14 h-14 rounded-full text-lg"
        />
      </div>
      <Pills
        {...props}
        ids={[
          { id: 'SELECT', label: 'Select' },
          { id: 'START', label: 'Start' },
        ]}
      />
    </div>
  );
}

/**
 * AtariButtons component renders the buttons for an Atari-style controller.
 * @param props - The properties for the AtariButtons component.
 * @returns The rendered AtariButtons component.
 */
function AtariButtons(props: { held: Held; setAction: SetAction }) {
  return (
    <div className="space-y-4">
      <div className="flex justify-center">
        <PadButton
          id="FIRE"
          label="FIRE"
          {...props}
          className="w-24 h-24 rounded-full text-sm tracking-wider"
        />
      </div>
      <Pills
        {...props}
        ids={[
          { id: 'SELECT', label: 'Select' },
          { id: 'RESET', label: 'Reset' },
        ]}
      />
    </div>
  );
}

const HEX_LAYOUT = ['1', '2', '3', 'C', '4', '5', '6', 'D', '7', '8', '9', 'E', 'A', '0', 'B', 'F'];

/**
 * HexPad component renders a hexadecimal pad for game controls.
 * @param props - The properties for the HexPad component.
 * @returns The rendered HexPad component.
 */
function HexPad(props: { held: Held; setAction: SetAction }) {
  return (
    <div className="grid grid-cols-4 gap-2">
      {HEX_LAYOUT.map((h) => (
        <PadButton
          key={h}
          id={h}
          label={h}
          {...props}
          className="w-14 h-14 rounded-xl text-lg font-mono"
        />
      ))}
    </div>
  );
}

/**
 * Props interface defines the properties for the TouchControls component.
 * @property system - The system definition for which the touch controls are being rendered.
 * @property enabled - A boolean indicating whether the touch controls are enabled.
 * @property disabled - An optional boolean indicating whether the touch controls are disabled.
 * @property onAction - A callback function that is called when a control action occurs.
 * @property children - The child elements to be rendered within the touch controls layout.
 */
interface Props {
  /** The system definition for which the touch controls are being rendered. */
  system: SystemDef | null;
  /** A boolean indicating whether the touch controls are enabled. */
  enabled: boolean;
  /** An optional boolean indicating whether the touch controls are disabled. */
  disabled?: boolean;
  /** A callback function that is called when a control action occurs. */
  onAction: (actionId: string, pressed: boolean) => void;
  /** The child elements to be rendered within the touch controls layout. */
  children: ReactNode;
}

/**
 * TouchControls component renders the touch controls layout for a given system, including directional pads and buttons.
 * It handles user interactions and invokes the provided onAction callback when control actions occur.
 * @param system - The system definition for which the touch controls are being rendered.
 * @param enabled - A boolean indicating whether the touch controls are enabled.
 * @param disabled - An optional boolean indicating whether the touch controls are disabled.
 * @param onAction - A callback function that is called when a control action occurs.
 * @param children - The child elements to be rendered within the touch controls layout.
 * @returns The rendered TouchControls component.
 */
export default function TouchControls({
  system,
  enabled,
  disabled = false,
  onAction,
  children,
}: Props) {
  const landscape = useMediaQuery('(orientation: landscape)');
  const heldRef = useRef(new Set<string>());
  const [held, setHeld] = useState<Held>(new Set());
  const onActionRef = useRef(onAction);

  useEffect(() => {
    onActionRef.current = onAction;
  }, [onAction]);

  const setAction = useCallback<SetAction>((id, pressed) => {
    if (heldRef.current.has(id) === pressed) return;
    if (pressed) heldRef.current.add(id);
    else heldRef.current.delete(id);
    setHeld(new Set(heldRef.current));
    onActionRef.current(id, pressed);
    if (pressed) navigator.vibrate?.(8);
  }, []);

  const releaseAll = useCallback(() => {
    for (const id of [...heldRef.current]) setAction(id, false);
  }, [setAction]);

  useEffect(() => {
    if (disabled || !enabled) releaseAll();
  }, [disabled, enabled, releaseAll]);

  useEffect(() => {
    const onHidden = () => {
      if (document.hidden) releaseAll();
    };
    window.addEventListener('blur', releaseAll);
    document.addEventListener('visibilitychange', onHidden);
    return () => {
      window.removeEventListener('blur', releaseAll);
      document.removeEventListener('visibilitychange', onHidden);
    };
  }, [releaseAll]);

  const touch = enabled && system !== null;

  const props = { held, setAction };
  let left: ReactNode = null;
  let right: ReactNode = null;

  if (touch && system) {
    if (system.id === 'chip8') {
      left = <HexPad {...props} />;
    } else {
      left = <DPad {...props} />;
      right = system.id === 'gameboy' ? <GameBoyButtons {...props} /> : <AtariButtons {...props} />;
    }
  }

  const wrapper: CSSProperties | undefined = !touch
    ? undefined
    : landscape
      ? {
          display: 'grid',
          alignItems: 'center',
          gap: 8,
          gridTemplateColumns: right ? 'auto minmax(0, 1fr) auto' : 'auto minmax(0, 1fr)',
          gridTemplateAreas: right ? '"left screen right"' : '"left screen"',
        }
      : {
          display: 'grid',
          gap: 12,
          gridTemplateColumns: right ? '1fr 1fr' : '1fr',
          gridTemplateAreas: right ? '"screen screen" "left right"' : '"screen" "left"',
        };

  const cluster = (area: string): CSSProperties => ({
    gridArea: area,
    justifySelf: 'center',
    opacity: disabled ? 0.45 : 1,
    pointerEvents: disabled ? 'none' : 'auto',
    transition: 'opacity 150ms',
  });

  return (
    <div className={touch ? 'bg-black pb-4 px-2' : undefined} style={wrapper}>
      <div style={{ gridArea: 'screen', minWidth: 0 }}>{children}</div>
      {left && <div style={cluster('left')}>{left}</div>}
      {right && <div style={cluster('right')}>{right}</div>}
    </div>
  );
}
