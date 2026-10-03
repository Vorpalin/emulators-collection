-- =====================================================================
--  Retro Assembly : script complet à exécuter dans Supabase
--  (profils, bibliothèque de jeux avec ROM dans Supabase Storage, réglages)
--  Dashboard > SQL Editor > New query > coller > Run
--  Rejouable sans risque (idempotent) : il peut être relancé après une
--  modification sans casser l'existant.
-- =====================================================================


-- ─────────────────────────────────────────────────────────────────────
-- 1. PROFILS  (id, username, email ; le mot de passe reste dans
--    auth.users, haché par Supabase : il n'a rien à faire ici)
-- ─────────────────────────────────────────────────────────────────────
create table if not exists public.profiles (
  id          uuid primary key references auth.users(id) on delete cascade,
  username    text not null check (username ~ '^[A-Za-z0-9_]{3,20}$'),
  email       text not null,
  created_at  timestamptz not null default now()
);

-- Pseudo unique sans tenir compte de la casse ("Bob" = "bob").
create unique index if not exists profiles_username_lower_idx
  on public.profiles (lower(username));


-- ─────────────────────────────────────────────────────────────────────
-- 2. FONCTION + TRIGGER : crée le profil à chaque inscription
--    Le pseudo vient de signUp({ options: { data: { username } } }).
-- ─────────────────────────────────────────────────────────────────────
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

-- Comptes créés AVANT ce script : on leur génère un profil.
insert into public.profiles (id, username, email)
select u.id, 'user_' || substr(u.id::text, 1, 8), u.email
from auth.users u
where not exists (select 1 from public.profiles p where p.id = u.id);


-- ─────────────────────────────────────────────────────────────────────
-- 3. FONCTION : pseudo disponible ? (appelée avant l'inscription)
--    Ne renvoie qu'un booléen, n'expose aucune donnée.
-- ─────────────────────────────────────────────────────────────────────
create or replace function public.is_username_available(name text)
returns boolean
language sql
stable
security definer
set search_path = public
as $$
  select not exists (select 1 from public.profiles where lower(username) = lower(name));
$$;

grant execute on function public.is_username_available(text) to anon, authenticated;


-- ─────────────────────────────────────────────────────────────────────
-- 4. SÉCURITÉ (RLS) DES PROFILS
-- ─────────────────────────────────────────────────────────────────────
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

-- DROITS (GRANT) : indispensables en plus du RLS. Sans eux, l'erreur est
-- "permission denied for table ..." (Supabase n'accorde plus l'accès
-- automatiquement aux nouvelles tables). On repart de zéro puis on donne
-- le strict nécessaire : lecture de son profil, modification du pseudo
-- uniquement (ni l'id, ni l'email).
revoke all on public.profiles from anon, authenticated;
grant select on public.profiles to authenticated;
grant update (username) on public.profiles to authenticated;


-- ─────────────────────────────────────────────────────────────────────
-- 5. BIBLIOTHÈQUE DE JEUX (privée, par utilisateur)
--    La ROM est stockée dans Supabase Storage (bucket "roms") : on garde ici
--    son chemin (rom_path). La colonne rom_url ne sert plus qu'aux jeux
--    ajoutés avant le retour au stockage Supabase : ils restent jouables.
-- ─────────────────────────────────────────────────────────────────────
create table if not exists public.games (
  id          uuid primary key default gen_random_uuid(),
  owner_id    uuid not null default auth.uid() references auth.users(id) on delete cascade,
  title       text not null,
  system      text not null check (system in ('chip8', 'atari2600', 'gameboy')),
  rom_path    text,       -- chemin dans le bucket "roms" : <user_id>/<fichier>
  size_bytes  integer,
  rom_url     text,       -- ancien mode : URL externe (lecture seule)
  created_at  timestamptz not null default now()
);
create index if not exists games_owner_idx on public.games (owner_id, created_at desc);

-- Mise à niveau d'une table existante, sans perdre de données (rejouable) :
-- fonctionne depuis la version "URL seule" comme depuis la toute première version.
alter table public.games add column if not exists rom_path   text;
alter table public.games add column if not exists size_bytes integer;
alter table public.games add column if not exists rom_url    text;
alter table public.games alter column rom_path   drop not null;
alter table public.games alter column size_bytes drop not null;
alter table public.games alter column rom_url    drop not null;

alter table public.games drop constraint if exists games_rom_url_check;
alter table public.games add constraint games_rom_url_check
  check (rom_url is null or rom_url ~* '^(https://|http://localhost)');

-- Un jeu doit avoir une ROM : dans Storage, ou (anciens jeux) à une URL.
alter table public.games drop constraint if exists games_has_rom;
alter table public.games add constraint games_has_rom
  check (rom_path is not null or rom_url is not null);

-- Garantit la valeur par défaut même si la table a été créée à la main : sans elle,
-- un INSERT qui n'envoie pas owner_id est refusé par la politique RLS.
alter table public.games alter column owner_id set default auth.uid();

alter table public.games enable row level security;

drop policy if exists "games: owner full access" on public.games;
create policy "games: owner full access" on public.games
  for all to authenticated
  using (owner_id = auth.uid())
  with check (owner_id = auth.uid());

-- Droits : un utilisateur connecté gère ses jeux ; l'anonyme n'a aucun accès.
revoke all on public.games from anon, authenticated;
grant select, insert, update, delete on public.games to authenticated;


-- ─────────────────────────────────────────────────────────────────────
-- 6. RÉGLAGES (touches, volume, filtre CRT)
-- ─────────────────────────────────────────────────────────────────────
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

-- Droits : lecture + enregistrement (upsert = insert + update) de ses réglages.
revoke all on public.user_settings from anon, authenticated;
grant select, insert, update on public.user_settings to authenticated;


-- ─────────────────────────────────────────────────────────────────────
-- 7. STOCKAGE DES ROM (bucket privé, un dossier par utilisateur)
--    Chaque utilisateur n'accède qu'à son dossier "<user_id>/...".
--    La limite de 16 Mo doit rester égale à MAX_ROM_BYTES dans lib/fetchRom.ts.
-- ─────────────────────────────────────────────────────────────────────
insert into storage.buckets (id, name, public, file_size_limit)
values ('roms', 'roms', false, 16777216)
on conflict (id) do update set public = false, file_size_limit = 16777216;

drop policy if exists "roms: read own" on storage.objects;
create policy "roms: read own" on storage.objects
  for select to authenticated
  using (bucket_id = 'roms' and (storage.foldername(name))[1] = auth.uid()::text);

drop policy if exists "roms: upload own" on storage.objects;
create policy "roms: upload own" on storage.objects
  for insert to authenticated
  with check (bucket_id = 'roms' and (storage.foldername(name))[1] = auth.uid()::text);

drop policy if exists "roms: delete own" on storage.objects;
create policy "roms: delete own" on storage.objects
  for delete to authenticated
  using (bucket_id = 'roms' and (storage.foldername(name))[1] = auth.uid()::text);
