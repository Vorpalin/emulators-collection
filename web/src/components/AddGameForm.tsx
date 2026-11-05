import { useRef, useState, type ChangeEvent, type FormEvent } from 'react';
import { Plus, Upload, X } from 'lucide-react';
import { ACCEPTED_EXTENSIONS, SYSTEM_LIST, detectSystem, type SystemId } from '../emulator/systems';
import { validateRomFile, type NewGame } from '../hooks/useGames';
import { useI18n } from '../i18n/I18nProvider';

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
  const { t } = useI18n();
  const fileInputRef = useRef<HTMLInputElement>(null);
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
      id="add-game-form"
      onSubmit={submit}
      aria-busy={busy}
      className="bg-slate-900 border border-slate-800 rounded-2xl p-5 space-y-3"
    >
      <div className="flex items-center justify-between">
        <h2 className="font-bold text-slate-200 text-sm">{t('addGameTitle')}</h2>
        <button
          type="button"
          onClick={onCancel}
          disabled={busy}
          aria-label={t('closeAddGame')}
          className="rounded-lg p-1 text-slate-500 transition hover:bg-slate-800 hover:text-slate-300 disabled:opacity-50"
        >
          <X className="w-4 h-4" />
        </button>
      </div>

      <label
        htmlFor="rom-file"
        className="flex cursor-pointer items-center gap-3 rounded-lg border border-dashed border-slate-700 bg-slate-950 px-3 py-3 transition hover:border-cyan-500"
      >
        <Upload className="w-4 h-4 text-cyan-400 shrink-0" />
        <span className="min-w-0 truncate text-sm text-slate-300" role="status" aria-live="polite">
          {file ? `${file.name} (${formatSize(file.size)})` : t('chooseRom')}
        </span>
        <input
          ref={fileInputRef}
          id="rom-file"
          type="file"
          required
          disabled={busy}
          aria-describedby="rom-file-help"
          accept={ACCEPTED_EXTENSIONS}
          onChange={onFileChange}
          className="sr-only"
          onClick={(event) => {
            event.currentTarget.value = '';
          }}
        />
      </label>

      <div className="grid sm:grid-cols-2 gap-3">
        <div>
          <label htmlFor="game-title" className="sr-only">
            {t('gameTitle')}
          </label>
          <input
            id="game-title"
            type="text"
            required
            disabled={busy}
            maxLength={80}
            placeholder={t('gameTitle')}
            aria-label={t('gameTitle')}
            value={title}
            onChange={(e) => {
              setTitle(e.target.value);
              setTitleTouched(true);
            }}
            className={input}
          />
        </div>
        <div>
          <label htmlFor="game-system" className="sr-only">
            {t('emulatorSystem')}
          </label>
          <select
            id="game-system"
            disabled={busy}
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
      </div>

      <p id="rom-file-help" className="text-xs text-slate-500">
        {t('acceptedFormats', { formats: ACCEPTED_EXTENSIONS.split(',').join(', ') })}
      </p>

      {error && (
        <p id="rom-file-error" role="alert" className="text-sm text-rose-400">
          {error}
        </p>
      )}

      <button
        type="submit"
        disabled={busy || !file || Boolean(file && validateRomFile(file)) || !title.trim()}
        className="bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 disabled:opacity-60 text-slate-950 font-semibold px-4 py-2 rounded-xl text-sm flex items-center space-x-2 transition"
      >
        <Plus className="w-4 h-4" />
        <span>{busy ? t('uploading') : t('add')}</span>
      </button>
    </form>
  );
}
