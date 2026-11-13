-- Move ROM binaries out of Supabase Storage and remove the public
-- username-availability RPC. Apply after deploying the matching frontend.
begin;

drop function if exists public.is_username_available(text);

alter table public.games drop constraint if exists games_has_rom;
alter table public.games drop constraint if exists games_rom_url_check;
alter table public.games drop column if exists rom_path;
alter table public.games drop column if exists rom_url;
-- Keep size_bytes as non-sensitive game metadata for the library UI.

-- Disable access to the legacy bucket. The bucket itself is intentionally
-- retained so existing files are not silently destroyed by this migration.
drop policy if exists "roms: read own" on storage.objects;
drop policy if exists "roms: upload own" on storage.objects;
drop policy if exists "roms: delete own" on storage.objects;

commit;
