import { useCallback, useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { deleteRom, getRom, listRomIds, saveRom } from '../lib/romStore';
import { deleteBattery } from '../lib/batteryStore';
import { deleteGame, getGame, listGames, saveGame } from '../lib/gameStore';
import type { SystemId } from '../emulator/systems';
import type { Game } from '../types';

/** Represents a new game to be added to the collection. */
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
 * Imports metadata for legacy ROMs already present in this browser.
 * Metadata belonging to ROMs stored by another browser is never imported.
 */
async function migrateLocalGameMetadata(): Promise<void> {
  const romIds = await listRomIds();
  if (romIds.length === 0) return;

  const localGames = await listGames();
  const knownIds = new Set(localGames.map((game) => game.id));
  const missingIds = romIds.filter((id) => !knownIds.has(id));
  if (missingIds.length === 0) return;

  // Keep requests bounded for accounts with large libraries.
  for (let offset = 0; offset < missingIds.length; offset += 100) {
    const ids = missingIds.slice(offset, offset + 100);
    const { data, error } = await supabase.from('games').select('*').in('id', ids);
    if (error) throw new Error(error.message);

    for (const game of (data ?? []) as Game[]) {
      await saveGame(game);
    }
  }
}

/**
 * Manages game metadata, ROM binaries and battery saves locally in this
 * browser. Supabase is only used once to migrate metadata for legacy local ROMs.
 */
export function useGames() {
  const [games, setGames] = useState<Game[]>([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState<string | null>(null);

  const refresh = useCallback(async () => {
    setLoading(true);
    try {
      // Migration is best-effort: already migrated local games remain usable
      // even if Supabase is temporarily unavailable.
      try {
        await migrateLocalGameMetadata();
      } catch (migrationError) {
        console.warn('Unable to migrate legacy local game metadata:', migrationError);
      }

      setGames(await listGames());
      setError(null);
    } catch (err) {
      setError(err instanceof Error ? err.message : String(err));
    } finally {
      setLoading(false);
    }
  }, []);

  useEffect(() => {
    void refresh();
  }, [refresh]);

  const add = useCallback(async ({ title, system, file }: NewGame): Promise<void> => {
    const fileError = validateRomFile(file);
    if (fileError) throw new Error(fileError);

    const { data: auth, error: authErr } = await supabase.auth.getUser();
    if (authErr || !auth.user) throw new Error('Your session has expired. Please sign in again.');

    const game: Game = {
      id: crypto.randomUUID(),
      owner_id: auth.user.id,
      title,
      system,
      size_bytes: file.size,
      created_at: new Date().toISOString(),
    };

    await saveRom(game.id, file);
    try {
      await saveGame(game);
    } catch (error) {
      await deleteRom(game.id).catch(() => undefined);
      throw error;
    }

    setGames((prev) => [game, ...prev]);
  }, []);

  const remove = useCallback(async (game: Game): Promise<void> => {
    // Do not delete the shared Supabase row: another browser may still have
    // its own local ROM and metadata for this game.
    await deleteRom(game.id);
    await deleteBattery(game.id).catch((storageError) => {
      console.warn('Could not delete the local battery save:', storageError);
    });
    await deleteGame(game.id);
    setGames((prev) => prev.filter((entry) => entry.id !== game.id));
  }, []);

  return { games, loading, error, add, remove, refresh };
}

/** Loads a ROM from this browser's IndexedDB store. */
export async function loadRom(game: Game): Promise<Uint8Array> {
  return getRom(game.id);
}

/** Loads game metadata from this browser's IndexedDB store. */
export function loadLocalGame(gameId: string): Promise<Game | undefined> {
  return getGame(gameId);
}
