import { useCallback, useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { useAuth } from '../auth/AuthProvider';
import { detectSystem } from '../emulator/systems';
import type { Game } from '../types';

const BUCKET = 'roms';

/** Bibliothèque de l'utilisateur : liste, ajout (upload ROM) et suppression. */
export function useGames() {
  const { user } = useAuth();
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

  /** Envoie la ROM dans Storage puis crée la ligne `games`. */
  const upload = useCallback(
    async (file: File): Promise<void> => {
      if (!user) throw new Error('Non connecté');
      const system = detectSystem(file.name);
      if (!system) throw new Error('Extension non supportée (.ch8, .a26, .gb).');

      // Les clés Storage n'acceptent pas tous les caractères.
      const safeName = file.name.replace(/[^\w.-]+/g, '_');
      const path = `${user.id}/${crypto.randomUUID()}-${safeName}`;

      const up = await supabase.storage.from(BUCKET).upload(path, file, {
        contentType: 'application/octet-stream',
      });
      if (up.error) throw new Error(up.error.message);

      const { data, error: err } = await supabase
        .from('games')
        .insert({
          title: file.name.replace(/\.[^/.]+$/, ''),
          system,
          rom_path: path,
          size_bytes: file.size,
        })
        .select()
        .single();
      if (err) {
        await supabase.storage.from(BUCKET).remove([path]); // pas d'orphelin
        throw new Error(err.message);
      }
      setGames((prev) => [data as Game, ...prev]);
    },
    [user],
  );

  const remove = useCallback(async (game: Game): Promise<void> => {
    const { error: err } = await supabase.from('games').delete().eq('id', game.id);
    if (err) throw new Error(err.message);
    await supabase.storage.from(BUCKET).remove([game.rom_path]);
    setGames((prev) => prev.filter((g) => g.id !== game.id));
  }, []);

  return { games, loading, error, upload, remove, refresh };
}

/** Télécharge les octets d'une ROM depuis le bucket privé. */
export async function downloadRom(game: Game): Promise<Uint8Array> {
  const { data, error } = await supabase.storage.from(BUCKET).download(game.rom_path);
  if (error || !data) throw new Error(error?.message ?? 'Téléchargement impossible');
  return new Uint8Array(await data.arrayBuffer());
}
