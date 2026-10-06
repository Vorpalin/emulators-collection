import { useCallback, useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { fetchRom, MAX_ROM_BYTES } from '../lib/fetchRom';
import type { SystemId } from '../emulator/systems';
import type { Game } from '../types';

const BUCKET = 'roms';

/**
 * Represents a new game to be added to the collection.
 * @interface NewGame
 * @property {string} title - The title of the game.
 * @property {SystemId} system - The system the game is for.
 * @property {File} file - The ROM file of the game.
 */
export interface NewGame {
  /**
   * The title of the game.
   */
  title: string;
  /**
   * The system the game is for.
   */
  system: SystemId;
  /**
   * The ROM file of the game.
   */
  file: File;
}

/**
 * Validates a ROM file based on its size.
 * @param file - The ROM file to validate.
 * @returns A string error message if the file is invalid, or null if the file is valid.
 */
export function validateRomFile(file: File): string | null {
  if (file.size === 0) return 'The file is empty.';
  if (file.size > MAX_ROM_BYTES) {
    return `The ROM is too large (maximum ${MAX_ROM_BYTES / 1024 / 1024} MB).`;
  }
  return null;
}

/**
 * A custom React hook for managing games in the application.
 * It provides functionalities to fetch, add, and remove games from the Supabase database.
 * It also handles loading and error states.
 *
 * @returns An object containing the list of games, loading state, error state, and functions to add, remove, and refresh games.
 */
export function useGames() {
  const [games, setGames] = useState<Game[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const refresh = useCallback(async () => {
    setLoading(true);
    const { data, error: err } = await supabase
      .from('games')
      .select('*')
      .order('created_at', { ascending: false });
    if (err) setError(err.message);
    else {
      setError(null);
      setGames(data as Game[]);
    }
    setLoading(false);
  }, []);

  useEffect(() => {
    void refresh();
  }, [refresh]);

  const add = useCallback(async ({ title, system, file }: NewGame): Promise<void> => {
    const fileError = validateRomFile(file);
    if (fileError) throw new Error(fileError);

    const { data: auth, error: authErr } = await supabase.auth.getUser();
    if (authErr || !auth.user) throw new Error('Your session has expired. Please sign in again.');

    const safeName = file.name.replace(/[^\w.-]+/g, '_');
    const path = `${auth.user.id}/${crypto.randomUUID()}-${safeName}`;

    const upload = await supabase.storage.from(BUCKET).upload(path, file, {
      contentType: 'application/octet-stream',
    });
    if (upload.error) throw new Error(upload.error.message);

    const { data, error: err } = await supabase
      .from('games')
      .insert({
        owner_id: auth.user.id,
        title,
        system,
        rom_path: path,
        size_bytes: file.size,
      })
      .select()
      .single();
    if (err) {
      await supabase.storage.from(BUCKET).remove([path]);
      throw new Error(err.message);
    }
    setGames((prev) => [data as Game, ...prev]);
  }, []);

  const remove = useCallback(async (game: Game): Promise<void> => {
    const { error: err } = await supabase.from('games').delete().eq('id', game.id);
    if (err) throw new Error(err.message);
    setGames((prev) => prev.filter((g) => g.id !== game.id));

    if (game.rom_path) {
      const { error: rmErr } = await supabase.storage.from(BUCKET).remove([game.rom_path]);
      if (rmErr) console.error('Could not delete the ROM file:', rmErr.message);
    }
  }, []);

  return { games, loading, error, add, remove, refresh };
}

/**
 * Loads the ROM data for a given game.
 * @param game - The game object containing information about the ROM.
 * @returns A promise that resolves to a Uint8Array containing the ROM data.
 */
export async function loadRom(game: Game): Promise<Uint8Array> {
  if (game.rom_path) {
    const { data, error } = await supabase.storage.from(BUCKET).download(game.rom_path);
    if (error || !data) throw new Error(error?.message ?? 'Could not download the ROM.');
    return new Uint8Array(await data.arrayBuffer());
  }
  if (game.rom_url) return fetchRom(game.rom_url);
  throw new Error('This game has no ROM attached.');
}
