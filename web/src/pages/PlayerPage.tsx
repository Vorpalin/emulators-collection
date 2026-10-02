import { useEffect, useRef, useState } from 'react';
import { Link, useParams } from 'react-router-dom';
import {
  AlertCircle,
  ArrowLeft,
  Maximize2,
  Pause,
  Play,
  RotateCcw,
  Volume2,
  VolumeX,
} from 'lucide-react';
import { supabase } from '../lib/supabase';
import { downloadRom } from '../hooks/useGames';
import { useSettings } from '../hooks/useSettings';
import { EmulatorSession } from '../emulator/session';
import { SYSTEMS } from '../emulator/systems';
import type { Game } from '../types';

type Status = 'loading' | 'ready' | 'playing' | 'error';

export default function PlayerPage() {
  const { gameId } = useParams();
  const { settings, loading: settingsLoading, update, bindingsFor } = useSettings();

  const [game, setGame] = useState<Game | null>(null);
  const [rom, setRom] = useState<Uint8Array | null>(null);
  const [status, setStatus] = useState<Status>('loading');
  const [error, setError] = useState<string | null>(null);

  const [paused, setPaused] = useState(false);
  const [muted, setMuted] = useState(false);
  const [volume, setVolume] = useState<number | null>(null); // null = valeur des réglages

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const frameRef = useRef<HTMLDivElement>(null);
  const sessionRef = useRef<EmulatorSession | null>(null);

  const effectiveVolume = volume ?? settings.volume;

  // 1) Récupère la fiche du jeu puis ses octets (bucket privé).
  useEffect(() => {
    let cancelled = false;
    setStatus('loading');
    (async () => {
      try {
        const { data, error: err } = await supabase
          .from('games')
          .select('*')
          .eq('id', gameId)
          .single();
        if (err) throw new Error(err.message);
        const g = data as Game;
        const bytes = await downloadRom(g);
        if (cancelled) return;
        setGame(g);
        setRom(bytes);
        setStatus('ready');
      } catch (e) {
        if (cancelled) return;
        setError(e instanceof Error ? e.message : String(e));
        setStatus('error');
      }
    })();
    return () => {
      cancelled = true;
    };
  }, [gameId]);

  // 2) Libère le cœur WASM et l'audio en quittant la page.
  useEffect(
    () => () => {
      sessionRef.current?.destroy();
      sessionRef.current = null;
    },
    [],
  );

  // Le démarrage doit venir d'un clic (politique d'autoplay audio).
  const start = async () => {
    if (!game || !rom || !canvasRef.current) return;
    const session = new EmulatorSession({
      canvas: canvasRef.current,
      system: SYSTEMS[game.system],
      bindings: bindingsFor(game.system),
      volume: effectiveVolume,
    });
    try {
      await session.start(rom);
      sessionRef.current = session;
      setStatus('playing');
    } catch (e) {
      session.destroy();
      setError(e instanceof Error ? e.message : String(e));
      setStatus('error');
    }
  };

  const togglePause = () => {
    const s = sessionRef.current;
    if (!s) return;
    if (s.isPaused) s.resume();
    else s.pause();
    setPaused(s.isPaused);
  };

  const toggleMute = () => {
    const next = !muted;
    setMuted(next);
    sessionRef.current?.setMuted(next);
  };

  const onVolume = (v: number) => {
    setVolume(v);
    setMuted(false);
    sessionRef.current?.setMuted(false);
    sessionRef.current?.setVolume(v);
  };

  if (status === 'error') {
    return (
      <div className="max-w-xl mx-auto mt-12 space-y-4">
        <div className="flex items-start space-x-3 bg-rose-500/10 border border-rose-500/40 text-rose-300 rounded-xl p-4 text-sm">
          <AlertCircle className="w-5 h-5 shrink-0 mt-0.5" />
          <span>{error}</span>
        </div>
        <Link to="/" className="text-sm text-cyan-400 hover:underline">
          ← Retour à la bibliothèque
        </Link>
      </div>
    );
  }

  const system = game ? SYSTEMS[game.system] : null;
  const aspect = system?.displayAspect ?? 1;
  const playing = status === 'playing';

  const bar = 'p-2 bg-slate-800 hover:bg-slate-700 disabled:opacity-50 text-slate-200 rounded-lg transition';

  return (
    <div className="space-y-4">
      <Link to="/" className="inline-flex items-center space-x-1.5 text-sm text-slate-400 hover:text-cyan-400">
        <ArrowLeft className="w-4 h-4" />
        <span>Bibliothèque</span>
      </Link>

      <div
        ref={frameRef}
        className="relative bg-slate-950 rounded-2xl border border-slate-800 shadow-2xl overflow-hidden flex flex-col"
      >
        {/* Barre de titre */}
        <div className="bg-slate-900/90 border-b border-slate-800 px-4 py-2.5 flex items-center justify-between">
          <div className="flex items-center space-x-3 min-w-0">
            <span
              className={`w-2.5 h-2.5 rounded-full ${playing && !paused ? 'bg-emerald-500 animate-pulse' : 'bg-slate-600'}`}
            />
            <span className="font-bold text-sm text-slate-200 truncate">
              {game?.title ?? 'Chargement…'}
            </span>
            {system && (
              <span className="bg-slate-800 text-slate-400 text-xs px-2 py-0.5 rounded font-mono">
                {system.label}
              </span>
            )}
          </div>
        </div>

        {/* Écran */}
        <div className="relative flex items-center justify-center p-4 bg-black min-h-[320px]">
          <div
            className="relative border-4 border-slate-800 rounded-lg overflow-hidden shadow-2xl"
            style={{ aspectRatio: String(aspect), width: `min(100%, calc(65vh * ${aspect}))` }}
          >
            <canvas ref={canvasRef} className="pixelated block w-full h-full" />

            {playing && settings.crt_filter && (
              <div
                className="pointer-events-none absolute inset-0 z-20"
                style={{
                  background:
                    'linear-gradient(rgba(18,16,16,0) 50%, rgba(0,0,0,0.25) 50%), linear-gradient(90deg, rgba(255,0,0,0.06), rgba(0,255,0,0.02), rgba(0,0,255,0.06))',
                  backgroundSize: '100% 4px, 6px 100%',
                }}
              />
            )}

            {paused && (
              <div className="absolute inset-0 bg-slate-950/70 flex flex-col items-center justify-center text-white z-30">
                <Pause className="w-12 h-12 text-amber-400 mb-2" />
                <span className="font-bold tracking-widest text-lg">PAUSE</span>
              </div>
            )}

            {!playing && (
              <div className="absolute inset-0 z-30 bg-slate-950 flex items-center justify-center">
                <button
                  onClick={() => void start()}
                  disabled={status !== 'ready' || settingsLoading}
                  className="bg-cyan-500 hover:bg-cyan-400 disabled:bg-slate-800 disabled:text-slate-500 text-slate-950 font-bold px-6 py-3 rounded-xl shadow-lg flex items-center space-x-2 transition"
                >
                  <Play className="w-5 h-5 fill-current" />
                  <span>{status === 'loading' || settingsLoading ? 'Chargement…' : 'LANCER'}</span>
                </button>
              </div>
            )}
          </div>
        </div>

        {/* Barre de contrôle */}
        <div className="bg-slate-900 border-t border-slate-800 p-3 flex flex-wrap items-center justify-between gap-3">
          <div className="flex items-center space-x-2">
            <button onClick={togglePause} disabled={!playing} className={bar} title={paused ? 'Reprendre' : 'Pause'}>
              {paused ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
            </button>
            <button onClick={() => sessionRef.current?.reset()} disabled={!playing} className={bar} title="Réinitialiser">
              <RotateCcw className="w-4 h-4" />
            </button>
          </div>

          <div className="flex items-center space-x-3">
            <button
              onClick={() => void update({ crt_filter: !settings.crt_filter })}
              className={`px-2.5 py-1 rounded-lg text-xs font-mono border transition ${
                settings.crt_filter
                  ? 'bg-cyan-500/10 border-cyan-500/40 text-cyan-400'
                  : 'bg-slate-950 border-slate-800 text-slate-500'
              }`}
            >
              CRT
            </button>

            <div className="flex items-center space-x-2">
              <button onClick={toggleMute} className="text-slate-400 hover:text-slate-200">
                {muted || effectiveVolume === 0 ? (
                  <VolumeX className="w-4 h-4 text-rose-400" />
                ) : (
                  <Volume2 className="w-4 h-4" />
                )}
              </button>
              <input
                type="range"
                min={0}
                max={100}
                value={muted ? 0 : effectiveVolume}
                onChange={(e) => onVolume(Number(e.target.value))}
                onPointerUp={() => void update({ volume: effectiveVolume })}
                className="w-20 accent-cyan-500 h-1 bg-slate-800 rounded"
              />
            </div>

            <button
              onClick={() => void frameRef.current?.requestFullscreen()}
              className={bar}
              title="Plein écran"
            >
              <Maximize2 className="w-4 h-4" />
            </button>
          </div>
        </div>
      </div>

      <p className="text-xs text-slate-500">
        Les touches se règlent dans l&apos;onglet « Contrôles ». Cliquez sur l&apos;écran si le clavier ne répond pas.
      </p>
    </div>
  );
}
