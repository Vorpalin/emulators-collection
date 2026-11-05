import { useMemo, useState } from 'react';
import { AlertCircle, Plus, Search, Sparkles, X, SlidersHorizontal } from 'lucide-react';
import AddGameForm from '../components/AddGameForm';
import GameCard from '../components/GameCard';
import { useGames } from '../hooks/useGames';
import { ACCEPTED_EXTENSIONS, SYSTEM_LIST, type SystemId } from '../emulator/systems';
import type { Game } from '../types';

type SortMode = 'recent' | 'oldest' | 'az' | 'za';

export default function LibraryPage() {
  const { games, loading, error, add, remove } = useGames();
  const [filter, setFilter] = useState<SystemId | 'all'>('all');
  const [query, setQuery] = useState('');
  const [sort, setSort] = useState<SortMode>('recent');
  const [adding, setAdding] = useState(false);
  const [actionError, setActionError] = useState<string | null>(null);
  const [deletingGameId, setDeletingGameId] = useState<string | null>(null);
  const [deleteMessage, setDeleteMessage] = useState<string | null>(null);

  const filtered = useMemo(() => {
    const normalizedQuery = query.trim().toLocaleLowerCase();
    return games
      .filter(
        (game) =>
          (filter === 'all' || game.system === filter) &&
          game.title.toLocaleLowerCase().includes(normalizedQuery),
      )
      .sort((a, b) => {
        if (sort === 'az') return a.title.localeCompare(b.title);
        if (sort === 'za') return b.title.localeCompare(a.title);
        const dateA = new Date(a.created_at).getTime();
        const dateB = new Date(b.created_at).getTime();
        return sort === 'oldest' ? dateA - dateB : dateB - dateA;
      });
  }, [games, filter, query, sort]);

  const onDelete = (game: Game) => {
    if (deletingGameId) return;

    if (window.confirm(`Delete « ${game.title} » from your library?`)) {
      setActionError(null);
      setDeleteMessage(null);
      setDeletingGameId(game.id);

      remove(game)
        .then(() => setDeleteMessage(`“${game.title}” was removed from your library.`))
        .catch((err) => setActionError(err instanceof Error ? err.message : String(err)))
        .finally(() => setDeletingGameId(null));
    }
  };

  const countForSystem = (systemId: SystemId | 'all') =>
    systemId === 'all' ? games.length : games.filter((game) => game.system === systemId).length;

  const pill = (active: boolean) =>
    `inline-flex min-h-10 items-center gap-2 rounded-xl border px-3.5 py-2 text-sm font-semibold whitespace-nowrap transition-colors focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-cyan-400 focus-visible:ring-offset-2 focus-visible:ring-offset-slate-950 ${
      active
        ? 'border-cyan-300 bg-cyan-300 text-slate-950 shadow-md shadow-cyan-950/30'
        : 'border-slate-600 bg-slate-800 text-slate-100 hover:border-slate-400 hover:bg-slate-700'
    }`;

  return (
    <div className="space-y-8">
      <section className="relative overflow-hidden rounded-3xl border border-slate-800 bg-slate-900/70 p-6 sm:p-8">
        <div className="pointer-events-none absolute -right-16 -top-24 h-64 w-64 rounded-full bg-cyan-500/10 blur-3xl" />
        <div className="pointer-events-none absolute -bottom-24 right-1/4 h-48 w-48 rounded-full bg-violet-500/10 blur-3xl" />
        <div className="relative flex flex-col justify-between gap-6 md:flex-row md:items-center">
          <div className="max-w-2xl">
            <div className="mb-3 inline-flex items-center gap-2 rounded-full border border-cyan-500/20 bg-cyan-500/10 px-3 py-1 text-xs font-semibold uppercase tracking-[0.16em] text-cyan-300">
              <Sparkles className="h-3.5 w-3.5" /> Your retro collection
            </div>
            <h1 className="text-3xl font-bold tracking-tight text-white sm:text-4xl">
              Game library
            </h1>
            <p className="mt-3 max-w-xl text-sm leading-6 text-slate-400">
              Your personal collection, ready to play. Add your own ROMs and launch them directly in
              your browser.
            </p>
            <div className="mt-5 flex flex-wrap gap-2 text-xs text-slate-500">
              <span className="rounded-lg border border-slate-800 bg-slate-950/60 px-3 py-1.5">
                {loading ? '…' : games.length} {games.length === 1 ? 'game' : 'games'}
              </span>
              <span className="rounded-lg border border-slate-800 bg-slate-950/60 px-3 py-1.5">
                {SYSTEM_LIST.length} systems supported
              </span>
            </div>
          </div>
          <button
            type="button"
            onClick={() => setAdding((value) => !value)}
            aria-expanded={adding}
            aria-controls="add-game-form"
            className="inline-flex shrink-0 items-center justify-center gap-2 rounded-xl bg-cyan-400 px-5 py-3 text-sm font-bold text-slate-950 shadow-lg shadow-cyan-500/15 transition hover:-translate-y-0.5 hover:bg-cyan-300 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-cyan-200"
          >
            <Plus className="h-4 w-4" /> {adding ? 'Close upload' : 'Add a game'}
          </button>
        </div>
      </section>

      {adding && <AddGameForm onSubmit={add} onCancel={() => setAdding(false)} />}

      {(error || actionError) && (
        <div
          role="alert"
          className="flex items-center gap-3 rounded-xl border border-rose-500/30 bg-rose-500/10 px-4 py-3 text-sm text-rose-300"
        >
          <AlertCircle className="h-4 w-4 shrink-0" />
          <span>{actionError ?? error}</span>
          {actionError && (
            <button
              className="ml-auto text-rose-200 underline"
              onClick={() => setActionError(null)}
            >
              Dismiss
            </button>
          )}
        </div>
      )}

      <section className="space-y-4">
        <div className="flex flex-col gap-4 xl:flex-row xl:items-center xl:justify-between">
          <div>
            <h2 className="text-lg font-semibold text-white">Browse games</h2>
            <p className="mt-1 text-sm text-slate-500" role="status" aria-live="polite">
              {loading
                ? 'Loading your collection…'
                : deletingGameId
                  ? 'Removing game…'
                  : deleteMessage ?? `Showing ${filtered.length} of ${games.length} games`}
            </p>
          </div>
          <div className="flex flex-col gap-3 sm:flex-row">
            <div className="relative min-w-0 sm:w-72">
              <Search className="pointer-events-none absolute left-3 top-1/2 h-4 w-4 -translate-y-1/2 text-slate-500" />
              <input
                type="search"
                placeholder="Search your collection…"
                value={query}
                onChange={(event) => setQuery(event.target.value)}
                aria-label="Search games"
                className="w-full rounded-xl border border-slate-800 bg-slate-900 py-2.5 pl-10 pr-10 text-sm text-slate-100 outline-none transition placeholder:text-slate-600 focus:border-cyan-500/60 focus:ring-2 focus:ring-cyan-500/10"
              />
              {query && (
                <button
                  type="button"
                  onClick={() => setQuery('')}
                  aria-label="Clear search"
                  className="absolute right-3 top-1/2 -translate-y-1/2 text-slate-500 hover:text-white"
                >
                  <X className="h-4 w-4" />
                </button>
              )}
            </div>
            <label className="flex items-center gap-2 rounded-xl border border-slate-800 bg-slate-900 px-3 text-sm text-slate-400">
              <SlidersHorizontal className="h-4 w-4" />
              <span className="sr-only">Sort games</span>
              <select
                value={sort}
                onChange={(event) => setSort(event.target.value as SortMode)}
                className="bg-transparent py-2.5 text-slate-200 outline-none"
              >
                <option value="recent">Recently added</option>
                <option value="oldest">Oldest first</option>
                <option value="az">Name A–Z</option>
                <option value="za">Name Z–A</option>
              </select>
            </label>
          </div>
        </div>

        <div className="rounded-2xl border border-slate-800 bg-slate-900/70 p-3 shadow-inner shadow-black/10">
          <div className="mb-2 flex items-center justify-between gap-3">
            <span className="text-xs font-bold uppercase tracking-wider text-slate-300">
              Filter by system
            </span>
            <span className="text-xs font-medium tabular-nums text-slate-400">
              {filter === 'all' ? games.length : filtered.length} shown
            </span>
          </div>
          <div
            className="rounded-2xl border border-slate-700 bg-slate-900/80 p-3 sm:p-4"
            role="group"
            aria-label="Filter games by system"
          >
            <div className="mb-3 flex items-center justify-between gap-3">
              <span className="text-xs font-bold uppercase tracking-wider text-slate-300">
                Filter by system
              </span>
              <span className="text-xs font-medium text-slate-400">
                {filter === 'all' ? 'All systems selected' : SYSTEM_LIST.find((system) => system.id === filter)?.label}
              </span>
            </div>
            <div className="flex flex-wrap gap-2">
              <button
                type="button"
                onClick={() => setFilter('all')}
                aria-pressed={filter === 'all'}
                className={pill(filter === 'all')}
              >
                All systems
                <span
                  className={`rounded-md px-1.5 py-0.5 text-xs tabular-nums ${
                    filter === 'all' ? 'bg-slate-950/15 text-slate-950' : 'bg-slate-950/50 text-slate-100'
                  }`}
                >
                  {countForSystem('all')}
                </span>
              </button>
              {SYSTEM_LIST.map((system) => {
                const active = filter === system.id;
                return (
                  <button
                    type="button"
                    key={system.id}
                    onClick={() => setFilter(system.id)}
                    aria-pressed={active}
                    className={pill(active)}
                  >
                    {system.label}
                    <span
                      className={`rounded-md px-1.5 py-0.5 text-xs tabular-nums ${
                        active ? 'bg-slate-950/15 text-slate-950' : 'bg-slate-950/50 text-slate-100'
                      }`}
                    >
                      {countForSystem(system.id)}
                    </span>
                  </button>
                );
              })}
            </div>
          </div>
        </div>

        {loading ? (
          <div className="grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-4">
            {Array.from({ length: 8 }, (_, index) => (
              <div
                key={index}
                className="h-56 animate-pulse rounded-2xl border border-slate-800 bg-slate-900/70"
              />
            ))}
          </div>
        ) : filtered.length === 0 ? (
          <div className="rounded-2xl border border-dashed border-slate-800 bg-slate-900/30 px-6 py-16 text-center">
            <div className="mx-auto mb-4 flex h-14 w-14 items-center justify-center rounded-2xl bg-slate-900 text-slate-500">
              <Search className="h-6 w-6" />
            </div>
            <h3 className="font-semibold text-slate-200">
              {games.length === 0 ? 'Your library is waiting' : 'No games found'}
            </h3>
            <p className="mx-auto mt-2 max-w-md text-sm text-slate-500">
              {games.length === 0
                ? `Add a ROM you own to get started. Supported formats: ${ACCEPTED_EXTENSIONS.split(',').join(', ')}.`
                : 'Try another search or system filter.'}
            </p>
            {games.length === 0 && (
              <button
                type="button"
                onClick={() => setAdding(true)}
                className="mt-5 inline-flex items-center gap-2 rounded-xl bg-cyan-400 px-4 py-2.5 text-sm font-semibold text-slate-950 hover:bg-cyan-300"
              >
                <Plus className="h-4 w-4" /> Add your first game
              </button>
            )}
            {(query || filter !== 'all') && (
              <button
                type="button"
                onClick={() => {
                  setQuery('');
                  setFilter('all');
                }}
                className="mt-4 block mx-auto text-sm text-cyan-300 hover:text-cyan-200"
              >
                Reset filters
              </button>
            )}
          </div>
        ) : (
          <div className="grid grid-cols-1 gap-4 sm:grid-cols-2 lg:grid-cols-3 2xl:grid-cols-4">
            {filtered.map((game) => (
              <GameCard
                key={game.id}
                game={game}
                onDelete={onDelete}
                deleting={deletingGameId === game.id}
              />
            ))}
          </div>
        )}
      </section>
    </div>
  );
}
