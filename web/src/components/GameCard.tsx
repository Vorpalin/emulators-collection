import { Link } from 'react-router-dom';
import { Play, Trash2, CalendarDays, HardDrive } from 'lucide-react';
import { SYSTEMS, type SystemId } from '../emulator/systems';
import type { Game } from '../types';
import { useI18n } from '../i18n/I18nProvider';

const ACCENTS: Record<SystemId, { gradient: string; glow: string; label: string }> = {
  chip8: {
    gradient: 'from-emerald-500/20 via-emerald-950/50 to-slate-950',
    glow: 'group-hover:border-emerald-400/40',
    label: 'text-emerald-300',
  },
  atari2600: {
    gradient: 'from-amber-500/20 via-amber-950/50 to-slate-950',
    glow: 'group-hover:border-amber-400/40',
    label: 'text-amber-300',
  },
  gameboy: {
    gradient: 'from-violet-500/20 via-violet-950/50 to-slate-950',
    glow: 'group-hover:border-violet-400/40',
    label: 'text-violet-300',
  },
  nes: {
    gradient: 'from-rose-500/20 via-rose-950/50 to-slate-950',
    glow: 'group-hover:border-rose-400/40',
    label: 'text-rose-300',
  },
};

function formatSize(bytes: number): string {
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(1)} KB`;
  return `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}

function sourceLabel(game: Game, t: (key: string) => string): string {
  if (game.size_bytes != null) return formatSize(game.size_bytes);
  if (game.rom_url) {
    try {
      return new URL(game.rom_url).host;
    } catch {
      return t('externalRom');
    }
  }
  return 'ROM';
}

export default function GameCard({
  game,
  onDelete,
  deleting = false,
}: {
  game: Game;
  onDelete: (game: Game) => void;
  deleting?: boolean;
}) {
  const { t, language } = useI18n();
  const system = SYSTEMS[game.system];
  const accent = ACCENTS[game.system] ?? ACCENTS.chip8;

  return (
    <article
      className={`group flex min-w-0 flex-col overflow-hidden rounded-2xl border border-slate-800 bg-slate-900/70 transition duration-300 hover:-translate-y-1 hover:shadow-xl hover:shadow-black/20 ${accent.glow}`}
    >
      <div
        className={`relative flex h-40 items-center justify-center overflow-hidden bg-gradient-to-br ${accent.gradient}`}
      >
        <div
          className="absolute inset-0 opacity-[0.08]"
          style={{
            backgroundImage:
              'linear-gradient(rgba(255,255,255,.3) 1px, transparent 1px), linear-gradient(90deg, rgba(255,255,255,.3) 1px, transparent 1px)',
            backgroundSize: '18px 18px',
          }}
        />
        <div className="relative text-center transition duration-300 group-hover:scale-105">
          <div
            className={`font-mono text-4xl font-black tracking-[0.2em] opacity-25 ${accent.label}`}
          >
            {system?.label?.toUpperCase() ?? 'ROM'}
          </div>
          <div className="mt-2 text-[10px] font-semibold uppercase tracking-[0.3em] text-slate-500">
            {t('emulatorCollection')}
          </div>
        </div>
        <span
          className={`absolute left-3 top-3 rounded-lg border border-white/10 bg-slate-950/70 px-2.5 py-1 text-[11px] font-semibold backdrop-blur ${accent.label}`}
        >
          {system?.label ?? t('unknownSystem')}
        </span>
        <span className="absolute right-3 top-3 inline-flex max-w-[45%] items-center gap-1.5 truncate rounded-lg border border-white/10 bg-slate-950/70 px-2.5 py-1 text-[11px] text-slate-400 backdrop-blur">
          <HardDrive className="h-3 w-3 shrink-0" />
          {sourceLabel(game, t)}
        </span>
        {system && (
          <div className="absolute inset-0 flex items-center justify-center bg-slate-950/70 opacity-0 backdrop-blur-sm transition duration-200 group-hover:opacity-100 group-focus-within:opacity-100">
            <Link
              to={`/play/${game.id}`}
              aria-label={`${t('playGame')} ${game.title}`}
              className="inline-flex items-center gap-2 rounded-xl bg-cyan-400 px-5 py-2.5 text-sm font-bold text-slate-950 shadow-lg shadow-cyan-500/20 transition hover:scale-105 hover:bg-cyan-300 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-white"
            >
              <Play className="h-4 w-4 fill-current" aria-hidden="true" />
              {t('playGame')}
            </Link>
          </div>
        )}
      </div>

      <div className="flex flex-1 items-start justify-between gap-3 p-4">
        <div className="min-w-0">
          <h3 className="truncate font-semibold text-slate-100" title={game.title}>
            {game.title}
          </h3>
          <p className="mt-2 flex items-center gap-1.5 text-xs text-slate-500">
            <CalendarDays className="h-3.5 w-3.5" aria-hidden="true" />
            <span>{t('added')} </span>
            <time dateTime={game.created_at}>
              {new Date(game.created_at).toLocaleDateString(language === 'fr' ? 'fr-FR' : 'en-US', {
                year: 'numeric',
                month: 'short',
                day: 'numeric',
              })}
            </time>
          </p>
        </div>
        <button
          type="button"
          onClick={() => onDelete(game)}
          title={
            deleting ? t('deleting', { title: game.title }) : t('deleteGame', { title: game.title })
          }
          aria-label={
            deleting ? t('deleting', { title: game.title }) : t('deleteGame', { title: game.title })
          }
          disabled={deleting}
          className="shrink-0 rounded-lg p-2 text-slate-600 transition hover:bg-rose-500/10 hover:text-rose-400 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-rose-400 disabled:cursor-wait disabled:opacity-50"
        >
          <Trash2 className={`h-4 w-4 ${deleting ? 'animate-pulse' : ''}`} />
        </button>
      </div>
    </article>
  );
}
