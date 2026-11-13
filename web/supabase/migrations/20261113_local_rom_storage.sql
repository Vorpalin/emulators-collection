-- Stop exposing the username-availability RPC and stop new ROM uploads.
-- Existing Storage objects and their read/delete policies are deliberately
-- preserved so users can export old ROMs before the bucket is removed.
begin;

drop function if exists public.is_username_available(text);

alter table public.games drop constraint if exists games_has_rom;
alter table public.games drop constraint if exists games_rom_url_check;

-- The new frontend no longer writes rom_path or rom_url. Keep the legacy
-- columns and stored files temporarily for a safe user-managed transition.

drop policy if exists "roms: upload own" on storage.objects;

commit;

-- After downloading/backing up any existing objects and re-importing them into
-- each browser's IndexedDB, remove the legacy read/delete policies, the roms
-- bucket and the rom_path/rom_url columns manually.
