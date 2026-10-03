import { useMemo, useState } from "react";
import { AlertCircle, Plus, Search, Sparkles } from "lucide-react";
import AddGameForm from "../components/AddGameForm";
import GameCard from "../components/GameCard";
import { useGames } from "../hooks/useGames";
import {
  ACCEPTED_EXTENSIONS,
  SYSTEM_LIST,
  type SystemId,
} from "../emulator/systems";
import type { Game } from "../types";

export default function LibraryPage() {
  const { games, loading, error, add, remove } = useGames();
  const [filter, setFilter] = useState<SystemId | "all">("all");
  const [query, setQuery] = useState("");
  const [adding, setAdding] = useState(false);
  const [actionError, setActionError] = useState<string | null>(null);

  const filtered = useMemo(
    () =>
      games.filter(
        (g) =>
          (filter === "all" || g.system === filter) &&
          g.title.toLowerCase().includes(query.toLowerCase()),
      ),
    [games, filter, query],
  );

  const onDelete = (game: Game) => {
    if (window.confirm(`Delete « ${game.title} » ?`)) {
      remove(game).catch((err) => setActionError(String(err)));
    }
  };

  const pill = (active: boolean) =>
    `px-3.5 py-1.5 rounded-lg text-xs font-semibold whitespace-nowrap transition ${
      active
        ? "bg-cyan-500 text-slate-950 shadow-md shadow-cyan-500/20"
        : "bg-slate-800 text-slate-300 hover:bg-slate-700"
    }`;

  return (
    <div className="space-y-6">
      <div className="bg-gradient-to-r from-slate-900 via-slate-900 to-purple-950/40 p-6 rounded-2xl border border-slate-800 flex flex-col md:flex-row items-start md:items-center justify-between gap-4">
        <div>
          <h1 className="text-2xl font-bold text-white flex items-center gap-2">
            <span>My library</span>
            <Sparkles className="w-5 h-5 text-amber-400" />
          </h1>
          <p className="text-slate-400 text-sm mt-1">
            Upload your ROMs ({ACCEPTED_EXTENSIONS.split(",").join(", ")}). They
            are stored privately in your account: only you can see and play
            them.
          </p>
        </div>
        <button
          onClick={() => setAdding((v) => !v)}
          className="bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-semibold px-4 py-2.5 rounded-xl shadow-lg shadow-cyan-500/20 transition flex items-center space-x-2 text-sm"
        >
          <Plus className="w-4 h-4" />
          <span>Add game</span>
        </button>
      </div>

      {adding && (
        <AddGameForm onSubmit={add} onCancel={() => setAdding(false)} />
      )}

      {(error || actionError) && (
        <div className="flex items-center space-x-2 bg-rose-500/10 border border-rose-500/40 text-rose-300 text-sm rounded-xl px-4 py-3">
          <AlertCircle className="w-4 h-4 shrink-0" />
          <span>{actionError ?? error}</span>
        </div>
      )}

      <div className="flex flex-col sm:flex-row justify-between items-center gap-4 bg-slate-900/60 p-3 rounded-xl border border-slate-800">
        <div className="flex items-center space-x-2 overflow-x-auto w-full sm:w-auto pb-2 sm:pb-0">
          <button
            onClick={() => setFilter("all")}
            className={pill(filter === "all")}
          >
            All
          </button>
          {SYSTEM_LIST.map((s) => (
            <button
              key={s.id}
              onClick={() => setFilter(s.id)}
              className={pill(filter === s.id)}
            >
              {s.label}
            </button>
          ))}
        </div>
        <div className="relative w-full sm:w-64">
          <Search className="w-4 h-4 text-slate-400 absolute left-3 top-2.5" />
          <input
            type="text"
            placeholder="Search…"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            className="w-full bg-slate-950 border border-slate-800 rounded-lg pl-9 pr-4 py-1.5 text-sm text-slate-200 placeholder-slate-500 focus:outline-none focus:border-cyan-500 transition"
          />
        </div>
      </div>

      {loading ? (
        <p className="text-center text-slate-500 text-sm font-mono py-12">
          Loading…
        </p>
      ) : filtered.length === 0 ? (
        <div className="text-center py-16 border border-dashed border-slate-800 rounded-2xl">
          <p className="text-slate-300 font-semibold">
            {games.length === 0
              ? "Your library is empty"
              : "No games match your search."}
          </p>
          <p className="text-slate-500 text-sm mt-1">
            {games.length === 0
              ? "Add a free-to-use ROM or one of your personal save files to get started."
              : "Try a different filter or search term."}
          </p>
        </div>
      ) : (
        <div className="grid grid-cols-1 sm:grid-cols-2 md:grid-cols-3 lg:grid-cols-5 gap-4">
          {filtered.map((g) => (
            <GameCard key={g.id} game={g} onDelete={onDelete} />
          ))}
        </div>
      )}
    </div>
  );
}
