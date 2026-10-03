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
  system      text not null check (system in ('chip8', 'atari2600', 'gameboy')),
  rom_path    text,
  size_bytes  integer,
  rom_url     text,
  created_at  timestamptz not null default now()
);
create index if not exists games_owner_idx on public.games (owner_id, created_at desc);

alter table public.games add column if not exists rom_path   text;
alter table public.games add column if not exists size_bytes integer;
alter table public.games add column if not exists rom_url    text;
alter table public.games alter column rom_path   drop not null;
alter table public.games alter column size_bytes drop not null;
alter table public.games alter column rom_url    drop not null;

alter table public.games drop constraint if exists games_rom_url_check;
alter table public.games add constraint games_rom_url_check
  check (rom_url is null or rom_url ~* '^(https://|http://localhost)');

alter table public.games drop constraint if exists games_has_rom;
alter table public.games add constraint games_has_rom
  check (rom_path is not null or rom_url is not null);

alter table public.games alter column owner_id set default auth.uid();

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
