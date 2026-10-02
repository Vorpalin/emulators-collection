# Retro Assembly — site web

Frontend **React + TypeScript + Tailwind** (Vite), authentification et stockage **Supabase**,
cœurs d'émulation **C++ compilés en WebAssembly** (Chip-8, Atari 2600, Game Boy).

## Architecture

```
web/
├── public/
│   ├── emu-audio-worklet.js   AudioWorklet : ring buffer PCM, renvoie le niveau de remplissage
│   └── wasm/                  emulators.js + emulators.wasm (généré, ignoré par git)
├── scripts/build-wasm.sh      compile le C++ (Emscripten) et copie la sortie dans public/wasm
├── supabase/schema.sql        tables, politiques RLS, bucket Storage privé
└── src/
    ├── main.tsx / App.tsx     routeur : /login · / · /play/:gameId · /controls
    ├── lib/supabase.ts        client Supabase (variables VITE_*)
    ├── auth/                  AuthProvider (session) · ProtectedRoute (redirige vers /login)
    ├── emulator/              ← tout ce qui touche au WASM, indépendant de React
    │   ├── systems.ts         consoles : extensions, actions/touches, touches par défaut
    │   ├── wasm.ts            types Embind + chargement du module
    │   └── session.ts         EmulatorSession : boucle, canvas, audio, clavier
    ├── hooks/                 useGames (liste/upload/suppression) · useSettings (touches, volume, CRT)
    ├── components/            Layout (header à onglets) · GameCard
    └── pages/                 LoginPage · LibraryPage · PlayerPage · ControlsPage
```

### Flux de données

```
Bibliothèque ──upload──▶ Storage (bucket privé "roms/<user_id>/…")  +  table games
Lecteur : games.id ─▶ download ROM ─▶ Uint8Array ─▶ emulator.load(system, rom)
                                                       │
   rAF ─▶ si audio en attente < 80 ms : stepFrame() ───┤
                                       ├─ framebuffer RGBA ─▶ <canvas>
                                       └─ échantillons PCM ─▶ AudioWorklet ─▶ haut-parleurs
   clavier ─▶ keyMap(code → indice) ─▶ emulator.setKey(i, pressed)
```

La vitesse de l'émulation est réglée par l'horloge audio (pas par `requestAnimationFrame`),
donc le son reste continu quel que soit le taux de rafraîchissement de l'écran.

## Mise en route

1. **Supabase** : créez un projet, exécutez `supabase/schema.sql` (SQL Editor).
   Dans *Authentication → URL Configuration*, ajoutez l'URL de votre site (et `http://localhost:5173`).
2. **Variables** : `cp .env.example .env` puis renseignez URL et clé *anon*.
3. **WebAssembly** (depuis la racine du dépôt C++, avec Emscripten activé) :
   `./web/scripts/build-wasm.sh`
4. **Site** : `cd web && npm install && npm run dev`

## Déploiement

Site 100 % statique (`npm run build` → `dist/`) : Vercel, Netlify, Cloudflare Pages…
Activez le *fallback SPA* (toutes les routes vers `index.html`) pour que `/play/…` fonctionne au rechargement.
Aucun en-tête COOP/COEP n'est nécessaire (pas de `SharedArrayBuffer`).

## Ajouter une console

1. C++ : implémenter `Console`, l'ajouter dans `src/wasm/bindings.cc` (`load`).
2. Web : ajouter une entrée dans `src/emulator/systems.ts` (id identique à celui de `bindings.cc`,
   extension, actions avec leurs indices de touche, touches par défaut) et dans le `check` SQL de `games.system`.

## Pas encore inclus

- **Save states** : le C++ n'expose pas encore de sérialisation d'état (`Console::saveState/loadState`).
  Une fois ajoutée, prévoir une table `save_states(game_id, slot, data)` ou des fichiers dans Storage.
- **Vitesse x2/x4**, **manette** (API Gamepad), **jaquettes** : faisables sans changer l'architecture.

## ROM

Chaque ROM est privée à son propriétaire (RLS + bucket privé). N'hébergez pas de ROM commerciales
dans un catalogue public : privilégiez le homebrew et le domaine public.
