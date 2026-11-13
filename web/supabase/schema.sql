-- Supabase schema for Emulators Collection.
-- ROM binaries are stored locally in each browser's IndexedDB database.
-- Supabase stores only account data, game metadata and user settings.

create table if not exists public.profiles (
  id          uuid primary key references auth.users(id) on delete cascade,
  username    text not null check (username ~ '^[A-Za-z0-9_]{3,20}$'),
  email       text not null,
  created_at  timestamptz not null default now()
);

create unique index if not exists profiles_username_lower_idx
  on public.profiles (lower(username));

create or replace function public.handle_new_user()
returns trigger
language plpgsql
security definer
set search_path = public
as $$
begin
  insert into public.profiles (id, username, email)
  values (
    new.id,
    coalesce(new.raw_user_meta_data->>'username', 'user_' || substr(new.id::text, 1, 8)),
    new.email
  );
  return new;
end;
$$;

drop trigger if exists on_auth_user_created on auth.users;
create trigger on_auth_user_created
  after insert on auth.users
  for each row execute function public.handle_new_user();

insert into public.profiles (id, username, email)
select u.id, 'user_' || substr(u.id::text, 1, 8), u.email
from auth.users u
where not exists (select 1 from public.profiles p where p.id = u.id);

-- Username availability is enforced by profiles_username_lower_idx.
-- Do not expose a public RPC for checking usernames: it enables enumeration.
drop function if exists public.is_username_available(text);

alter table public.profiles enable row level security;

drop policy if exists "profiles: read own" on public.profiles;
create policy "profiles: read own" on public.profiles
  for select to authenticated
  using (id = auth.uid());

drop policy if exists "profiles: update own" on public.profiles;
create policy "profiles: update own" on public.profiles
  for update to authenticated
  using (id = auth.uid())
  with check (id = auth.uid());

revoke all on public.profiles from anon, authenticated;
grant select on public.profiles to authenticated;
grant update (username) on public.profiles to authenticated;

create table if not exists public.games (
  id          uuid primary key default gen_random_uuid(),
  owner_id    uuid not null default auth.uid() references auth.users(id) on delete cascade,
  title       text not null,
  system      text not null check (system in ('chip8', 'atari2600', 'gameboy', 'nes')),
  size_bytes  bigint,
  created_at  timestamptz not null default now()
);
create index if not exists games_owner_idx on public.games (owner_id, created_at desc);

alter table public.games add column if not exists size_bytes bigint;
alter table public.games alter column owner_id set default auth.uid();

alter table public.games drop constraint if exists games_rom_url_check;
alter table public.games drop constraint if exists games_has_rom;
alter table public.games drop column if exists rom_path;
alter table public.games drop column if exists rom_url;

-- Allow the NES on databases created before it was supported.
alter table public.games drop constraint if exists games_system_check;
alter table public.games add constraint games_system_check
  check (system in ('chip8', 'atari2600', 'gameboy', 'nes'));

alter table public.games enable row level security;

drop policy if exists "games: owner full access" on public.games;
create policy "games: owner full access" on public.games
  for all to authenticated
  using (owner_id = auth.uid())
  with check (owner_id = auth.uid());

revoke all on public.games from anon, authenticated;
grant select, insert, update, delete on public.games to authenticated;

create table if not exists public.user_settings (
  user_id      uuid primary key default auth.uid() references auth.users(id) on delete cascade,
  key_bindings jsonb not null default '{}',
  crt_filter   boolean not null default true,
  volume       smallint not null default 80 check (volume between 0 and 100),
  updated_at   timestamptz not null default now()
);

alter table public.user_settings enable row level security;

drop policy if exists "settings: owner full access" on public.user_settings;
create policy "settings: owner full access" on public.user_settings
  for all to authenticated
  using (user_id = auth.uid())
  with check (user_id = auth.uid());

revoke all on public.user_settings from anon, authenticated;
grant select, insert, update on public.user_settings to authenticated;

-- The legacy 'roms' Storage bucket is intentionally not recreated.
-- Remove it from the Supabase dashboard after reviewing any existing files.
