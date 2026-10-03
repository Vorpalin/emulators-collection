import { Link } from "react-router-dom";
import { Play, Trash2 } from "lucide-react";
import { SYSTEMS, type SystemId } from "../emulator/systems";
import type { Game } from "../types";

const GRADIENTS: Record<SystemId, string> = {
  chip8: "from-emerald-900/70 to-slate-950",
  atari2600: "from-amber-900/70 to-slate-950",
  gameboy: "from-purple-900/70 to-slate-950",
};

function hostOf(url: string): string {
  try {
    return new URL(url).host;
  } catch {
    return "?";
  }
}

function formatSize(bytes: number): string {
  return bytes < 1024 * 1024
    ? `${(bytes / 1024).toFixed(1)} KB`
    : `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}

/** Taille du fichier, ou domaine d'origine pour un jeu ajouté par URL. */
function sourceLabel(game: Game): string {
  if (game.size_bytes != null) return formatSize(game.size_bytes);
  if (game.rom_url) return hostOf(game.rom_url);
  return "";
}

export default function GameCard({
  game,
  onDelete,
}: {
  game: Game;
  onDelete: (g: Game) => void;
}) {
  const system = SYSTEMS[game.system];

  return (
    <div className="group bg-slate-900 rounded-xl border border-slate-800/80 overflow-hidden hover:border-cyan-500/50 hover:shadow-xl hover:shadow-cyan-500/10 transition-all duration-300 flex flex-col">
      <div
        className={`relative h-36 bg-gradient-to-br ${GRADIENTS[game.system]} flex items-center justify-center`}
      >
        <span className="text-3xl font-extrabold tracking-widest text-white/20 select-none">
          {system.label.toUpperCase()}
        </span>
        <div className="absolute top-2 left-2 bg-slate-950/80 backdrop-blur text-cyan-400 text-[10px] font-mono px-2 py-0.5 rounded border border-cyan-500/30">
          {system.label}
        </div>
        <div className="absolute top-2 right-2 bg-slate-950/80 backdrop-blur text-slate-300 text-[10px] font-mono px-2 py-0.5 rounded border border-slate-800 max-w-[55%] truncate">
          {sourceLabel(game)}
        </div>

        <div className="absolute inset-0 bg-slate-950/60 backdrop-blur-sm opacity-0 group-hover:opacity-100 transition-opacity flex items-center justify-center">
          <Link
            to={`/play/${game.id}`}
            className="bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold px-4 py-2 rounded-lg shadow-lg flex items-center space-x-2"
          >
            <Play className="w-4 h-4 fill-current" />
            <span>PLAY</span>
          </Link>
        </div>
      </div>

      <div className="p-3.5 flex items-center justify-between gap-2">
        <div className="min-w-0">
          <h3 className="font-bold text-slate-100 text-sm truncate">
            {game.title}
          </h3>
          <p className="text-xs text-slate-500 mt-0.5">
            Added on {new Date(game.created_at).toLocaleDateString()}
          </p>
        </div>
        <button
          onClick={() => onDelete(game)}
          title="Delete"
          className="p-1.5 rounded-lg text-slate-500 hover:text-rose-400 hover:bg-slate-800 transition"
        >
          <Trash2 className="w-4 h-4" />
        </button>
      </div>
    </div>
  );
}
