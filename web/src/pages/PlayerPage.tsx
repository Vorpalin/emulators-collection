import { useEffect, useRef, useState } from 'react';
import { Link, useParams } from 'react-router-dom';
import {
  AlertCircle,
  ArrowLeft,
  Download,
  FolderOpen,
  Maximize2,
  Pause,
  Play,
  RotateCcw,
  Upload,
  Volume2,
  VolumeX,
} from 'lucide-react';
import { supabase } from '../lib/supabase';
import { loadRom } from '../hooks/useGames';
import { useSettings } from '../hooks/useSettings';
import TouchControls, { useIsTouchDevice } from '../components/TouchControls';
import { EmulatorSession } from '../emulator/session';
import { SYSTEMS } from '../emulator/systems';
import type { Game } from '../types';

type Status = 'loading' | 'ready' | 'playing' | 'error';

export default function PlayerPage() {
  const { gameId } = useParams();
  const { settings, loading: settingsLoading, update, bindingsFor } = useSettings();
  const isTouch = useIsTouchDevice();

  const [game, setGame] = useState<Game | null>(null);
  const [rom, setRom] = useState<Uint8Array | null>(null);
  const [status, setStatus] = useState<Status>('loading');
  const [error, setError] = useState<string | null>(null);

  const [paused, setPaused] = useState(false);
  const [muted, setMuted] = useState(false);
  const [volume, setVolume] = useState<number | null>(null);
  const [stateLoading, setStateLoading] = useState(false);

  const canvasRef = useRef<HTMLCanvasElement>(null);
  const frameRef = useRef<HTMLDivElement>(null);
  const sessionRef = useRef<EmulatorSession | null>(null);
  const stateInputRef = useRef<HTMLInputElement>(null);

  const effectiveVolume = volume ?? settings.volume;

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
        const bytes = await loadRom(g);

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

  useEffect(
    () => () => {
      sessionRef.current?.destroy();
      sessionRef.current = null;
    },
    [],
  );

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
      setPaused(false);
      setStatus('playing');
    } catch (e) {
      session.destroy();
      setError(e instanceof Error ? e.message : String(e));
      setStatus('error');
    }
  };

  const pressAction = (actionId: string, pressed: boolean) =>
    sessionRef.current?.setActionPressed(actionId, pressed);

  const togglePause = () => {
    const s = sessionRef.current;
    if (!s) return;

    if (s.isPaused) {
      s.resume();
    } else {
      s.pause();
    }

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

  const sanitizeFilename = (name: string) =>
    name
      .replace(/[<>:"/\\|?*]/g, '_')
      .replace(/\s+/g, '_')
      .slice(0, 100);

  const saveState = () => {
    const session = sessionRef.current;

    if (!session || !game) return;

    try {
      const state = session.saveState();

      const blob = new Blob([state], {
        type: 'application/json',
      });

      const url = URL.createObjectURL(blob);
      const anchor = document.createElement('a');

      anchor.href = url;
      anchor.download = `${sanitizeFilename(game.title)}.state.json`;
      document.body.appendChild(anchor);
      anchor.click();
      anchor.remove();

      URL.revokeObjectURL(url);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
      setStatus('error');
    }
  };

  const openStatePicker = () => {
    if (!sessionRef.current) return;

    stateInputRef.current?.click();
  };

  const loadState = async (event: React.ChangeEvent<HTMLInputElement>) => {
    const file = event.target.files?.[0];
    const session = sessionRef.current;

    // Allow selecting the same file again later.
    event.target.value = '';

    if (!file || !session) return;

    setStateLoading(true);

    try {
      const data = await file.text();

      const wasPaused = session.isPaused;

      if (!wasPaused) {
        session.pause();
      }

      const success = session.loadState(data);

      if (!success) {
        throw new Error('The save state could not be loaded.');
      }

      if (!wasPaused) {
        session.resume();
      }

      setPaused(session.isPaused);
    } catch (e) {
      setError(e instanceof Error ? e.message : String(e));
      setStatus('error');
    } finally {
      setStateLoading(false);
    }
  };

  if (status === 'error') {
    return (
      <div className="max-w-xl mx-auto mt-12 space-y-4">
        <div className="flex items-start space-x-3 bg-rose-500/10 border border-rose-500/40 text-rose-300 rounded-xl p-4 text-sm">
          <AlertCircle className="w-5 h-5 shrink-0 mt-0.5" />
          <span>{error}</span>
        </div>

        <Link to="/" className="text-sm text-cyan-400 hover:underline">
          ← Return to the library
        </Link>
      </div>
    );
  }

  const system = game ? SYSTEMS[game.system] : null;
  const aspect = system?.displayAspect ?? 1;
  const playing = status === 'playing';

  const bar =
    'p-2 bg-slate-800 hover:bg-slate-700 disabled:opacity-50 text-slate-200 rounded-lg transition';

  return (
    <div className="space-y-4">
      <Link
        to="/"
        className="inline-flex items-center space-x-1.5 text-sm text-slate-400 hover:text-cyan-400"
      >
        <ArrowLeft className="w-4 h-4" />
        <span>Library</span>
      </Link>

      <div
        ref={frameRef}
        className="relative bg-slate-950 rounded-2xl border border-slate-800 shadow-2xl overflow-hidden flex flex-col"
      >
        {/* Title bar */}
        <div className="bg-slate-900/90 border-b border-slate-800 px-4 py-2.5 flex items-center justify-between">
          <div className="flex items-center space-x-3 min-w-0">
            <span
              className={`w-2.5 h-2.5 rounded-full ${
                playing && !paused ? 'bg-emerald-500 animate-pulse' : 'bg-slate-600'
              }`}
            />

            <span className="font-bold text-sm text-slate-200 truncate">
              {game?.title ?? 'Loading…'}
            </span>

            {system && (
              <span className="bg-slate-800 text-slate-400 text-xs px-2 py-0.5 rounded font-mono">
                {system.label}
              </span>
            )}
          </div>
        </div>

        {/* Screen (+ touch controls on mobile) */}
        <TouchControls
          system={system}
          enabled={isTouch}
          disabled={!playing || paused}
          onAction={pressAction}
        >
          <div className="relative flex items-center justify-center p-4 bg-black min-h-[320px]">
            <div
              className="relative border-4 border-slate-800 rounded-lg overflow-hidden shadow-2xl"
              style={{
                aspectRatio: String(aspect),
                width: `min(100%, calc(65vh * ${aspect}))`,
              }}
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
                    <span>{status === 'loading' || settingsLoading ? 'Loading…' : 'START'}</span>
                  </button>
                </div>
              )}
            </div>
          </div>
        </TouchControls>

        {/* Control bar */}
        <div className="bg-slate-900 border-t border-slate-800 p-3 flex flex-wrap items-center justify-between gap-3">
          <div className="flex items-center space-x-2">
            {/* Pause */}
            <button
              onClick={togglePause}
              disabled={!playing}
              className={bar}
              title={paused ? 'Continue' : 'Pause'}
            >
              {paused ? <Play className="w-4 h-4" /> : <Pause className="w-4 h-4" />}
            </button>

            {/* Reset */}
            <button
              onClick={() => sessionRef.current?.reset()}
              disabled={!playing}
              className={bar}
              title="Restart"
            >
              <RotateCcw className="w-4 h-4" />
            </button>

            {/* Save state */}
            <button
              onClick={saveState}
              disabled={!playing || stateLoading}
              className={bar}
              title="Save"
            >
              <Download className="w-4 h-4" />
            </button>

            {/* Load state */}
            <button
              onClick={openStatePicker}
              disabled={!playing || stateLoading}
              className={bar}
              title="Upload"
            >
              {stateLoading ? (
                <FolderOpen className="w-4 h-4 animate-pulse" />
              ) : (
                <Upload className="w-4 h-4" />
              )}
            </button>

            <input
              ref={stateInputRef}
              type="file"
              accept=".json,application/json"
              onChange={(event) => void loadState(event)}
              className="hidden"
            />
          </div>

          <div className="flex items-center space-x-3">
            {/* CRT */}
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

            {/* Volume */}
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

            {/* Fullscreen */}
            {document.fullscreenEnabled && (
              <button
                onClick={() => void frameRef.current?.requestFullscreen?.()?.catch(() => {})}
                className={bar}
                title="Fullscreen"
              >
                <Maximize2 className="w-4 h-4" />
              </button>
            )}
          </div>
        </div>
      </div>

      <p className="text-xs text-slate-500">
        {isTouch
          ? 'Use the on-screen buttons to play. Rotate your phone or use fullscreen for a bigger screen.'
          : 'The keys are configured in the « Controls » tab. Click on the screen if the keyboard does not respond.'}
      </p>
    </div>
  );
}
