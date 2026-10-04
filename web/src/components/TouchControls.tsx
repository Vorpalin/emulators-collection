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

export const useIsTouchDevice = () => useMediaQuery('(pointer: coarse)');

type SetAction = (id: string, pressed: boolean) => void;
type Held = ReadonlySet<string>;

const noGestures: CSSProperties = {
  touchAction: 'none',
  userSelect: 'none',
  WebkitUserSelect: 'none',
  WebkitTouchCallout: 'none',
};

const idle = 'bg-slate-700 text-slate-200';
const active = 'bg-cyan-400 text-slate-950';

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

interface Props {
  system: SystemDef | null;
  enabled: boolean;
  disabled?: boolean;
  onAction: (actionId: string, pressed: boolean) => void;
  children: ReactNode;
}

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
    if (heldRef.current.has(id) === pressed) return; // évite les doublons
    if (pressed) heldRef.current.add(id);
    else heldRef.current.delete(id);
    setHeld(new Set(heldRef.current));
    onActionRef.current(id, pressed);
    if (pressed) navigator.vibrate?.(8); // retour haptique (Android)
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

  if (!enabled || !system) return <>{children}</>;

  const props = { held, setAction };
  let left: ReactNode;
  let right: ReactNode = null;

  if (system.id === 'chip8') {
    left = <HexPad {...props} />;
  } else {
    left = <DPad {...props} />;
    right = system.id === 'gameboy' ? <GameBoyButtons {...props} /> : <AtariButtons {...props} />;
  }

  const wrapper: CSSProperties = landscape
    ? {
        display: 'grid',
        gridTemplateColumns: `auto minmax(0, 1fr)${right ? ' auto' : ''}`,
        alignItems: 'center',
        gap: 8,
      }
    : {
        display: 'grid',
        gridTemplateColumns: right ? '1fr 1fr' : '1fr',
        gap: 12,
      };

  const cluster: CSSProperties = {
    justifySelf: 'center',
    opacity: disabled ? 0.45 : 1,
    pointerEvents: disabled ? 'none' : 'auto',
    transition: 'opacity 150ms',
  };

  const screen = (
    <div style={landscape ? { minWidth: 0 } : { gridColumn: '1 / -1' }}>{children}</div>
  );

  return (
    <div className="bg-black pb-4 px-2" style={wrapper}>
      {landscape ? (
        <>
          <div style={cluster}>{left}</div>
          {screen}
          {right && <div style={cluster}>{right}</div>}
        </>
      ) : (
        <>
          {screen}
          <div style={cluster}>{left}</div>
          {right && <div style={cluster}>{right}</div>}
        </>
      )}
    </div>
  );
}
