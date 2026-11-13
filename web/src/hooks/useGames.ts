import { useCallback, useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { deleteRom, getRom, saveRom } from '../lib/romStore';
import type { SystemId } from '../emulator/systems';
import type { Game } from '../types';

/**
 * Represents a new game to be added to the collection.
 */
export interface NewGame {
  title: string;
  system: SystemId;
  file: File;
}

/**
 * Validates that an imported ROM contains data. Its maximum size is determined
 * by the browser's available IndexedDB quota rather than a fixed app limit.
 */
export function validateRomFile(file: File): string | null {
  if (file.size === 0) return 'The file is empty.';
  return null;
}

/**
 * Manages the user's game metadata in Supabase and ROM binaries in local
 * browser storage. ROM files never leave the browser.
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

    const { data, error: err } = await supabase
      .from('games')
      .insert({
        owner_id: auth.user.id,
        title,
        system,
        size_bytes: file.size,
      })
      .select()
      .single();

    if (err) throw new Error(err.message);

    try {
      await saveRom(data.id, file);
    } catch (storageError) {
      const { error: rollbackError } = await supabase.from('games').delete().eq('id', data.id);
      if (rollbackError) {
        console.error(
          'Could not roll back game metadata after ROM storage failed:',
          rollbackError.message,
        );
      }
      throw storageError;
    }

    setGames((prev) => [data as Game, ...prev]);
  }, []);

  const remove = useCallback(async (game: Game): Promise<void> => {
    const { error: err } = await supabase.from('games').delete().eq('id', game.id);
    if (err) throw new Error(err.message);

    setGames((prev) => prev.filter((g) => g.id !== game.id));

    try {
      await deleteRom(game.id);
    } catch (storageError) {
      console.warn('Could not delete the local ROM file:', storageError);
    }
  }, []);

  return { games, loading, error, add, remove, refresh };
}

/** Loads a ROM from this browser's IndexedDB store. */
export async function loadRom(game: Game): Promise<Uint8Array> {
  return getRom(game.id);
}
