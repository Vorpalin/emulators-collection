import { useCallback, useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import type { SystemId } from '../emulator/systems';
import type { Game } from '../types';

export interface NewGame {
  title: string;
  system: SystemId;
  url: string;
}

/** Valide une URL de ROM : HTTPS obligatoire (http autorisé seulement en local). */
export function validateRomUrl(raw: string): string | null {
  let u: URL;
  try {
    u = new URL(raw);
  } catch {
    return 'URL invalide.';
  }
  const local = u.protocol === 'http:' && u.hostname === 'localhost';
  if (u.protocol !== 'https:' && !local) return "L'URL doit commencer par https://";
  return null;
}

/** Bibliothèque de l'utilisateur : liste, ajout (par URL) et suppression. */
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

  /** Enregistre un jeu : seule l'URL est conservée (owner_id = auth.uid() par défaut). */
  const add = useCallback(async (game: NewGame): Promise<void> => {
    const urlError = validateRomUrl(game.url);
    if (urlError) throw new Error(urlError);

    const { data, error: err } = await supabase
      .from('games')
      .insert({ title: game.title, system: game.system, rom_url: game.url })
      .select()
      .single();
    if (err) throw new Error(err.message);
    setGames((prev) => [data as Game, ...prev]);
  }, []);

  const remove = useCallback(async (game: Game): Promise<void> => {
    const { error: err } = await supabase.from('games').delete().eq('id', game.id);
    if (err) throw new Error(err.message);
    setGames((prev) => prev.filter((g) => g.id !== game.id));
  }, []);

  return { games, loading, error, add, remove, refresh };
}
