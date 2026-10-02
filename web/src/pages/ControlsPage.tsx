import { useEffect, useState } from 'react';
import { Gamepad2, RotateCcw } from 'lucide-react';
import { useSettings } from '../hooks/useSettings';
import { SYSTEM_LIST, SYSTEMS, type SystemId } from '../emulator/systems';

/** "KeyA" -> "A", "ArrowUp" -> "↑", "Digit3" -> "3"… */
function prettyKey(code: string): string {
  if (code.startsWith('Key')) return code.slice(3);
  if (code.startsWith('Digit')) return code.slice(5);
  const names: Record<string, string> = {
    ArrowUp: '↑',
    ArrowDown: '↓',
    ArrowLeft: '←',
    ArrowRight: '→',
    Space: 'Espace',
    ShiftRight: 'Maj droite',
    ShiftLeft: 'Maj gauche',
  };
  return names[code] ?? code;
}

export default function ControlsPage() {
  const { settings, loading, update, bindingsFor } = useSettings();
  const [systemId, setSystemId] = useState<SystemId>('gameboy');
  const [listening, setListening] = useState<string | null>(null); // action.id en attente d'une touche

  const system = SYSTEMS[systemId];
  const bindings = bindingsFor(systemId);

  const save = (next: Record<string, string>) =>
    update({ key_bindings: { ...settings.key_bindings, [systemId]: next } });

  // Capture la prochaine touche pressée pendant la réaffectation.
  useEffect(() => {
    if (!listening) return;
    const onKey = (e: KeyboardEvent) => {
      e.preventDefault();
      if (e.code === 'Escape') {
        setListening(null);
        return;
      }
      const next = { ...bindings };
      // Si la touche est déjà prise par une autre action, on échange.
      const clash = Object.keys(next).find((id) => next[id] === e.code && id !== listening);
      if (clash) next[clash] = next[listening];
      next[listening] = e.code;
      void save(next);
      setListening(null);
    };
    window.addEventListener('keydown', onKey);
    return () => window.removeEventListener('keydown', onKey);
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [listening, bindings]);

  const pill = (active: boolean) =>
    `px-3.5 py-1.5 rounded-lg text-xs font-semibold whitespace-nowrap transition ${
      active
        ? 'bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20'
        : 'bg-slate-800 text-slate-300 hover:bg-slate-700'
    }`;

  return (
    <div className="space-y-6 max-w-3xl">
      <div>
        <h1 className="text-2xl font-bold text-white flex items-center gap-2">
          <Gamepad2 className="w-6 h-6 text-cyan-400" />
          <span>Contrôles</span>
        </h1>
        <p className="text-sm text-slate-400 mt-1">
          Cliquez sur une touche pour la réaffecter. Les réglages sont enregistrés sur votre compte.
        </p>
      </div>

      <div className="flex items-center space-x-2 overflow-x-auto">
        {SYSTEM_LIST.map((s) => (
          <button
            key={s.id}
            onClick={() => {
              setSystemId(s.id);
              setListening(null);
            }}
            className={pill(systemId === s.id)}
          >
            {s.label}
          </button>
        ))}
      </div>

      <div className="bg-slate-900 p-6 rounded-2xl border border-slate-800 space-y-4">
        <div className="flex items-center justify-between">
          <h2 className="font-bold text-slate-200">{system.label}</h2>
          <button
            onClick={() => {
              setListening(null);
              void save({ ...system.defaultBindings });
            }}
            className="text-xs bg-slate-800 hover:bg-slate-700 text-slate-300 px-3 py-1.5 rounded-lg border border-slate-700 flex items-center space-x-1.5"
          >
            <RotateCcw className="w-3.5 h-3.5" />
            <span>Valeurs par défaut</span>
          </button>
        </div>

        {listening && (
          <div className="bg-cyan-500/10 border border-cyan-500 p-3 rounded-xl flex items-center space-x-3">
            <span className="w-3 h-3 rounded-full bg-cyan-400 animate-ping" />
            <span className="text-sm font-bold text-cyan-300">
              Appuyez sur une touche pour « {system.actions.find((a) => a.id === listening)?.label} »
              (Échap pour annuler)
            </span>
          </div>
        )}

        <div className={`grid gap-2 ${system.actions.length > 8 ? 'sm:grid-cols-2' : ''}`}>
          {system.actions.map((action) => (
            <div
              key={action.id}
              className="flex items-center justify-between p-2.5 bg-slate-950 rounded-lg border border-slate-800/80"
            >
              <span className="text-xs font-mono text-slate-300 font-bold">{action.label}</span>
              <button
                disabled={loading}
                onClick={() => setListening(action.id)}
                className={`text-xs font-mono px-2.5 py-1 rounded border transition ${
                  listening === action.id
                    ? 'bg-cyan-500 text-slate-950 border-cyan-400'
                    : 'bg-slate-800 hover:bg-slate-700 text-cyan-400 border-slate-700'
                }`}
              >
                {bindings[action.id] ? prettyKey(bindings[action.id]) : '—'}
              </button>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}
