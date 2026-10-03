import type { SystemId } from './emulator/systems';

export interface Game {
  id: string;
  owner_id: string;
  title: string;
  system: SystemId;
  rom_path: string | null;
  size_bytes: number | null;
  rom_url: string | null;
  created_at: string;
}

export interface Settings {
  key_bindings: Partial<Record<SystemId, Record<string, string>>>;
  crt_filter: boolean;
  volume: number;
}
