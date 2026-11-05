import { useEffect, useState } from 'react';
import { Gamepad2, RotateCcw, Keyboard, CheckCircle2, AlertCircle } from 'lucide-react';
import { useSettings } from '../hooks/useSettings';
import { SYSTEM_LIST, SYSTEMS, type SystemId } from '../emulator/systems';
import { useKeyLabel } from '../emulator/keyboardLayout';

export default function ControlsPage() {
  const { settings, loading, update, bindingsFor, defaultsFor } = useSettings();
  const keyLabel = useKeyLabel();
  const [systemId, setSystemId] = useState<SystemId>('gameboy');
  const [listening, setListening] = useState<string | null>(null);
  const [saveError, setSaveError] = useState<string | null>(null);

  const system = SYSTEMS[systemId];
  const bindings = bindingsFor(systemId);

  const save = async (next: Record<string, string>) => {
    setSaveError(null);
    try {
      await update({ key_bindings: { ...settings.key_bindings, [systemId]: next } });
    } catch (err) {
      setSaveError(err instanceof Error ? err.message : String(err));
    }
  };

  useEffect(() => {
    if (!listening) return;
    const onKey = (event: KeyboardEvent) => {
      event.preventDefault();
      event.stopPropagation();
      if (event.code === 'Escape') {
        setListening(null);
        return;
      }
      if (event.code === 'Tab' || event.code === 'MetaLeft' || event.code === 'MetaRight') return;
      const next = { ...bindings };
      const conflict = Object.keys(next).find((id) => next[id] === event.code && id !== listening);
      if (conflict) next[conflict] = next[listening];
      next[listening] = event.code;
      void save(next);
      setListening(null);
    };
    window.addEventListener('keydown', onKey, true);
    return () => window.removeEventListener('keydown', onKey, true);
  }, [listening, bindings]);

  const systemButton = (active: boolean) =>
    `rounded-xl px-4 py-2.5 text-sm font-semibold whitespace-nowrap transition focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-cyan-400 ${
      active
        ? 'bg-cyan-400 text-slate-950 shadow-lg shadow-cyan-500/15'
        : 'border border-slate-800 bg-slate-900 text-slate-400 hover:border-slate-700 hover:text-white'
    }`;

  return (
    <div className="mx-auto max-w-4xl space-y-7">
      <header className="relative overflow-hidden rounded-3xl border border-slate-800 bg-slate-900/70 p-6 sm:p-8">
        <div className="pointer-events-none absolute -right-16 -top-20 h-56 w-56 rounded-full bg-violet-500/10 blur-3xl" />
        <div className="relative flex items-start gap-4">
          <div className="rounded-2xl bg-violet-500/10 p-3 text-violet-300 ring-1 ring-violet-400/20">
            <Gamepad2 className="h-6 w-6" />
          </div>
          <div>
            <p className="text-xs font-semibold uppercase tracking-[0.18em] text-violet-300">
              Personal setup
            </p>
            <h1 className="mt-1 text-3xl font-bold tracking-tight text-white">Controls</h1>
            <p className="mt-2 max-w-xl text-sm leading-6 text-slate-400">
              Configure keyboard bindings for each system. Changes are synced to your account.
            </p>
          </div>
        </div>
      </header>

      {saveError && (
        <div
          role="alert"
          className="flex items-center gap-2 rounded-xl border border-rose-500/30 bg-rose-500/10 p-3 text-sm text-rose-300"
        >
          <AlertCircle className="h-4 w-4" />
          {saveError}
        </div>
      )}

      <section className="space-y-3">
        <h2 className="text-sm font-semibold uppercase tracking-wider text-slate-500">
          Choose a system
        </h2>
        <div className="flex gap-2 overflow-x-auto pb-1">
          {SYSTEM_LIST.map((item) => (
            <button
              key={item.id}
              onClick={() => {
                setSystemId(item.id);
                setListening(null);
              }}
              className={systemButton(systemId === item.id)}
            >
              {item.label}
            </button>
          ))}
        </div>
      </section>

      <section className="overflow-hidden rounded-2xl border border-slate-800 bg-slate-900/60">
        <div className="flex flex-col gap-3 border-b border-slate-800 p-5 sm:flex-row sm:items-center sm:justify-between">
          <div>
            <h2 className="font-semibold text-white">{system.label} keyboard layout</h2>
            <p className="mt-1 text-xs text-slate-500">
              {system.actions.length} actions · Click a key to remap
            </p>
          </div>
          <button
            type="button"
            disabled={loading}
            onClick={() => {
              setListening(null);
              void save(defaultsFor(systemId));
            }}
            className="inline-flex items-center justify-center gap-2 rounded-xl border border-slate-800 bg-slate-950 px-3.5 py-2 text-xs font-semibold text-slate-300 transition hover:border-slate-700 hover:text-white disabled:opacity-50"
          >
            <RotateCcw className="h-3.5 w-3.5" /> Restore defaults
          </button>
        </div>

        {listening && (
          <div
            role="status"
            className="mx-5 mt-5 flex items-center gap-3 rounded-xl border border-cyan-400/30 bg-cyan-400/10 p-3.5 text-sm text-cyan-200"
          >
            <span className="relative flex h-2.5 w-2.5">
              <span className="absolute inline-flex h-full w-full animate-ping rounded-full bg-cyan-400 opacity-75" />
              <span className="relative inline-flex h-2.5 w-2.5 rounded-full bg-cyan-400" />
            </span>
            Press a key for{' '}
            <strong>{system.actions.find((action) => action.id === listening)?.label}</strong>.
            Escape cancels.
          </div>
        )}

        <div className={`grid gap-2 p-5 ${system.actions.length > 8 ? 'sm:grid-cols-2' : ''}`}>
          {system.actions.map((action) => {
            const active = listening === action.id;
            return (
              <div
                key={action.id}
                className={`flex items-center justify-between gap-3 rounded-xl border p-3 transition ${active ? 'border-cyan-400/40 bg-cyan-400/5' : 'border-slate-800 bg-slate-950/60 hover:border-slate-700'}`}
              >
                <div className="flex min-w-0 items-center gap-3">
                  <div className="flex h-8 w-8 shrink-0 items-center justify-center rounded-lg bg-slate-900 text-slate-500">
                    <Keyboard className="h-4 w-4" />
                  </div>
                  <span className="truncate text-sm font-medium text-slate-300">
                    {action.label}
                  </span>
                </div>
                <button
                  type="button"
                  disabled={loading}
                  title={bindings[action.id] ?? 'Unassigned'}
                  onClick={() => setListening(action.id)}
                  className={`min-w-[76px] rounded-lg border px-3 py-2 font-mono text-xs font-semibold transition focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-cyan-400 disabled:opacity-50 ${active ? 'border-cyan-300 bg-cyan-400 text-slate-950' : 'border-slate-700 bg-slate-800 text-cyan-300 hover:border-cyan-400/40 hover:bg-slate-700'}`}
                >
                  {bindings[action.id] ? keyLabel(bindings[action.id]) : '—'}
                </button>
              </div>
            );
          })}
        </div>
        <div className="flex items-center gap-2 border-t border-slate-800 px-5 py-3 text-xs text-slate-500">
          <CheckCircle2 className="h-3.5 w-3.5 text-emerald-400" /> Keyboard changes are saved
          automatically.
        </div>
      </section>
    </div>
  );
}
