import type { SystemId } from "./emulator/systems";

/** Ligne de la table `games`. */
export interface Game {
  id: string;
  owner_id: string;
  title: string;
  system: SystemId;
  /** ROM stockée dans Supabase Storage (bucket "roms") : chemin "<user_id>/<fichier>". */
  rom_path: string | null;
  size_bytes: number | null;
  /** Ancien mode : ROM hébergée ailleurs. Les jeux déjà ajoutés ainsi restent jouables. */
  rom_url: string | null;
  created_at: string;
}

/** Ligne de la table `user_settings` (sans les colonnes techniques). */
export interface Settings {
  key_bindings: Partial<Record<SystemId, Record<string, string>>>;
  crt_filter: boolean;
  volume: number;
}
