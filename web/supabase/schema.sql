-- À exécuter dans Supabase > SQL Editor (après schema.sql).
-- Table des profils : id, username, email. Le mot de passe reste dans auth.users (haché).

create table public.profiles (
  id          uuid primary key references auth.users(id) on delete cascade,
  username    text not null check (username ~ '^[A-Za-z0-9_]{3,20}$'),
  email       text not null,   -- copie informative de auth.users.email
  created_at  timestamptz not null default now()
);

-- Pseudo unique sans tenir compte de la casse ("Bob" et "bob" = même pseudo).
create unique index profiles_username_lower_idx on public.profiles (lower(username));

-- ───────────── Création automatique du profil à l'inscription ─────────────
-- Le pseudo vient de signUp({ options: { data: { username } } }).
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

create trigger on_auth_user_created
  after insert on auth.users
  for each row execute function public.handle_new_user();

-- ───────────── Sécurité (RLS) ─────────────
alter table public.profiles enable row level security;

create policy "profiles: read own" on public.profiles
  for select to authenticated
  using (id = auth.uid());

create policy "profiles: update own" on public.profiles
  for update to authenticated
  using (id = auth.uid())
  with check (id = auth.uid());

-- Un utilisateur ne peut modifier que son pseudo (pas l'email ni l'id).
revoke update on public.profiles from authenticated;
grant update (username) on public.profiles to authenticated;

-- ───────────── Pseudo disponible ? (appelable avant l'inscription) ─────────────
-- Ne renvoie qu'un booléen : n'expose aucune donnée de la table.
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
