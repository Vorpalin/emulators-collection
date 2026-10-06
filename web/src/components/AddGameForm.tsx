import { useState, type ChangeEvent, type FormEvent } from 'react';
import { Plus, Upload, X } from 'lucide-react';
import { ACCEPTED_EXTENSIONS, SYSTEM_LIST, detectSystem, type SystemId } from '../emulator/systems';
import { validateRomFile, type NewGame } from '../hooks/useGames';

/**
 * Props interface defines the properties for the AddGameForm component.
 * @property onSubmit - A function to handle form submission.
 * @property onCancel - A function to handle form cancellation.
 */
interface Props {
  /** A function to handle form submission. */
  onSubmit: (game: NewGame) => Promise<void>;
  /** A function to handle form cancellation. */
  onCancel: () => void;
}

/**
 * Generates a title from the given file name by removing its extension.
 * @param name The name of the file from which to generate the title.
 * @returns The generated title.
 */
function titleFromFile(name: string): string {
  return name.replace(/\.[^/.]+$/, '');
}

/**
 * Formats the given size in bytes into a human-readable string.
 * @param bytes The size in bytes to format.
 * @returns The formatted size.
 */
function formatSize(bytes: number): string {
  return bytes < 1024 * 1024
    ? `${(bytes / 1024).toFixed(1)} KB`
    : `${(bytes / 1024 / 1024).toFixed(1)} MB`;
}

/**
 * AddGameForm component allows users to add a new game by uploading a ROM file, specifying a title, and selecting a system.
 * @param param0  The properties for the AddGameForm component.
 * @returns The AddGameForm component.
 */
export default function AddGameForm({ onSubmit, onCancel }: Props) {
  const [file, setFile] = useState<File | null>(null);
  const [title, setTitle] = useState('');
  const [system, setSystem] = useState<SystemId>('gameboy');
  const [titleTouched, setTitleTouched] = useState(false);
  const [systemTouched, setSystemTouched] = useState(false);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const onFileChange = (e: ChangeEvent<HTMLInputElement>) => {
    const picked = e.target.files?.[0] ?? null;
    setFile(picked);
    setError(picked ? validateRomFile(picked) : null);
    if (!picked) return;
    if (!titleTouched) setTitle(titleFromFile(picked.name));
    const detected = detectSystem(picked.name);
    if (detected && !systemTouched) setSystem(detected);
  };

  const submit = async (e: FormEvent) => {
    e.preventDefault();
    if (!file) return;
    setBusy(true);
    setError(null);
    try {
      await onSubmit({ file, title: title.trim(), system });
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
    <form
      onSubmit={submit}
      className="bg-slate-900 border border-slate-800 rounded-2xl p-5 space-y-3"
    >
      <div className="flex items-center justify-between">
        <h2 className="font-bold text-slate-200 text-sm">Add a game</h2>
        <button type="button" onClick={onCancel} className="text-slate-500 hover:text-slate-300">
          <X className="w-4 h-4" />
        </button>
      </div>

      <label className="flex items-center gap-3 cursor-pointer bg-slate-950 border border-dashed border-slate-700 hover:border-cyan-500 rounded-lg px-3 py-3 transition">
        <Upload className="w-4 h-4 text-cyan-400 shrink-0" />
        <span className="text-sm text-slate-300 truncate">
          {file ? `${file.name} (${formatSize(file.size)})` : 'Choose a ROM file…'}
        </span>
        <input
          type="file"
          required
          accept={ACCEPTED_EXTENSIONS}
          onChange={onFileChange}
          className="sr-only"
          aria-label="ROM file"
        />
      </label>

      <div className="grid sm:grid-cols-2 gap-3">
        <input
          type="text"
          required
          maxLength={80}
          placeholder="Title"
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
        Accepted formats: {ACCEPTED_EXTENSIONS.split(',').join(', ')}. Maximum 16 MB. The file is
        stored privately in your account: only you can play it.
      </p>

      {error && <p className="text-sm text-rose-400">{error}</p>}

      <button
        type="submit"
        disabled={busy || !file}
        className="bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 disabled:opacity-60 text-slate-950 font-semibold px-4 py-2 rounded-xl text-sm flex items-center space-x-2 transition"
      >
        <Plus className="w-4 h-4" />
        <span>{busy ? 'Uploading…' : 'Add'}</span>
      </button>
    </form>
  );
}
