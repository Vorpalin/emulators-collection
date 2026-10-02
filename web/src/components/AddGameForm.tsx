import { useState, type FormEvent } from 'react';
import { Plus, X } from 'lucide-react';
import { SYSTEM_LIST, detectSystem, type SystemId } from '../emulator/systems';
import type { NewGame } from '../hooks/useGames';

interface Props {
  onSubmit: (game: NewGame) => Promise<void>;
  onCancel: () => void;
}

function titleFromUrl(raw: string): string {
  try {
    const name = decodeURIComponent(new URL(raw).pathname.split('/').pop() ?? '');
    return name.replace(/\.[^/.]+$/, '');
  } catch {
    return '';
  }
}

export default function AddGameForm({ onSubmit, onCancel }: Props) {
  const [url, setUrl] = useState('');
  const [title, setTitle] = useState('');
  const [system, setSystem] = useState<SystemId>('gameboy');
  const [titleTouched, setTitleTouched] = useState(false);
  const [systemTouched, setSystemTouched] = useState(false);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const onUrlChange = (value: string) => {
    setUrl(value);
    if (!titleTouched) setTitle(titleFromUrl(value));
    const detected = detectSystem(value.split(/[?#]/)[0]);
    if (detected && !systemTouched) setSystem(detected);
  };

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    setBusy(true);
    setError(null);
    try {
      await onSubmit({ url: url.trim(), title: title.trim(), system });
      onCancel();
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setBusy(false);
    }
  };

  const input =
    'w-full bg-slate-950 border border-slate-800 rounded-lg px-3 py-2 text-sm text-slate-200 placeholder-slate-500 focus:outline-none focus:border-cyan-500 transition';

  return (
    <form onSubmit={submit} className="bg-slate-900 border border-slate-800 rounded-2xl p-5 space-y-3">
      <div className="flex items-center justify-between">
        <h2 className="font-bold text-slate-200 text-sm">Add a game by URL</h2>
        <button type="button" onClick={onCancel} className="text-slate-500 hover:text-slate-300">
          <X className="w-4 h-4" />
        </button>
      </div>

      <input
        type="url"
        required
        placeholder="https://exemple.com/roms/mon-jeu.gb"
        value={url}
        onChange={(e) => onUrlChange(e.target.value)}
        className={input}
      />

      <div className="grid sm:grid-cols-2 gap-3">
        <input
          type="text"
          required
          maxLength={80}
          placeholder="Titre"
          value={title}
          onChange={(e) => {
            setTitle(e.target.value);
            setTitleTouched(true);
          }}
          className={input}
        />
        <select
          value={system}
          onChange={(e) => {
            setSystem(e.target.value as SystemId);
            setSystemTouched(true);
          }}
          className={input}
        >
          {SYSTEM_LIST.map((s) => (
            <option key={s.id} value={s.id}>
              {s.label}
            </option>
          ))}
        </select>
      </div>

      <p className="text-xs text-slate-500">
        The file is not copied to this site: it is downloaded from this address each time it is launched. The server must be in HTTPS and allow this site (CORS).
      </p>

      {error && <p className="text-sm text-rose-400">{error}</p>}

      <button
        type="submit"
        disabled={busy}
        className="bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 disabled:opacity-60 text-slate-950 font-semibold px-4 py-2 rounded-xl text-sm flex items-center space-x-2 transition"
      >
        <Plus className="w-4 h-4" />
        <span>{busy ? 'Adding…' : 'Add'}</span>
      </button>
    </form>
  );
}
