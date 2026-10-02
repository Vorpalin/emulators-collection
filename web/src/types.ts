import type { SystemId } from './emulator/systems';

/** Ligne de la table `games`. Aucune ROM n'est stockée : seulement son URL. */
export interface Game {
  id: string;
  owner_id: string;
  title: string;
  system: SystemId;
  rom_url: string; // adresse externe (HTTPS), téléchargée par le navigateur
  created_at: string;
}

/** Ligne de la table `user_settings` (sans les colonnes techniques). */
export interface Settings {
  key_bindings: Partial<Record<SystemId, Record<string, string>>>;
  crt_filter: boolean;
  volume: number;
}
