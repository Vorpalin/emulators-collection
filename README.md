# Emulators Collection

[![CI](https://github.com/Vorpalin/emulators-collection/actions/workflows/checks.yml/badge.svg)](https://github.com/Vorpalin/emulators-collection/actions/workflows/checks.yml)
[![Security](https://github.com/Vorpalin/emulators-collection/actions/workflows/security.yml/badge.svg)](https://github.com/Vorpalin/emulators-collection/actions/workflows/security.yml)
[![License](https://img.shields.io/github/license/Vorpalin/emulators-collection)](LICENSE)
[![XERJ Corpus Hub contributor](https://hub.xerj.org/contributors/Vorpalin/badge.svg)](https://hub.xerj.org/contributors/Vorpalin)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-Emscripten-654FF0.svg)](https://webassembly.org/)
[![React](https://img.shields.io/badge/React-TypeScript-61DAFB.svg)](https://react.dev/)
[![Vite](https://img.shields.io/badge/Vite-frontend-646CFF.svg)](https://vite.dev/)

**Emulators Collection** is a modular collection of classic computer and video game system emulators written in **C++20** and compiled to **WebAssembly**.

The emulator cores are independent from the browser and expose a common interface. A **React + TypeScript + Vite** frontend connects these cores to the browser, providing video output, audio, input handling, a personal game library and save management.

> **One emulator architecture — multiple systems — one browser interface.**

## 🌐 Play in the browser

**[▶ Launch Emulators Collection](https://vorpalin.github.io/emulators-collection/)**

The emulators run directly in the browser through WebAssembly. No native emulator installation is required.

---

## 🎮 Supported Systems

| System | Status | Hardware / Features |
| --- | :---: | --- |
| **CHIP-8** | ✅ | CHIP-8 interpreter, 64×32 display |
| **Super-CHIP 48** | ✅ | Extended CHIP-8 instructions, 128×64 display |
| **Atari 2600** | ✅ | MOS 6507, MOS 6532 RIOT, TIA, cartridges |
| **Game Boy / Game Boy Color** | ✅ | Sharp LR35902, PPU, timer, interrupts, joypad, APU, cartridges with battery-backed RAM. Game Boy Color (`.gbc`, detected from the cartridge header): colour palettes, VRAM/WRAM banking, double-speed mode, HDMA |
| **NES** | ✅ | Ricoh 2A03 (6502), 2C02 PPU, 5-channel APU, mappers 0, 1, 2, 3, 4 (MMC3), 7, 66 (`.nes`), battery-backed RAM |

The project is designed so that additional systems can be integrated without changing the architecture of the existing emulators. Accuracy is tracked with community test ROMs: see [Tests](#-tests) for what passes today and what is a known limitation.

---

## ✨ Features

### Emulator core

- Modular C++20 architecture
- Common `Console` interface
- Independent emulator cores
- Reusable hardware components (the 6502 core is shared by the Atari 2600 and the NES)
- CPU, bus, PPU, timer, controller, cartridge and audio abstractions
- Save states (JSON) and battery-backed cartridge RAM (`.sav` images)
- Strict compiler warnings
- CMake-based build system, native and WebAssembly

### WebAssembly

- C++ emulator cores compiled with **Emscripten** (`-O3`, LTO)
- Embind bindings (`src/wasm/bindings.cc`)
- ROM loading, frame execution, input forwarding
- Direct access to the framebuffer and audio samples in WebAssembly memory

### Web application

- React, TypeScript and Vite
- Canvas rendering with display options (CRT scanlines and curvature, soft blur, integer scaling, Game Boy palettes, device frames)
- Keyboard, gamepad and on-screen touch controls, with remappable keys per system
- Web Audio output through an `AudioWorklet`; the emulation falls back to a wall-clock timer when the browser blocks audio
- Personal **game library stored in the browser** (IndexedDB): ROMs, game metadata and in-game saves never leave the device
- Accounts with **Supabase** (sign-in, password reset, profile and synced key bindings)
- Installable **PWA** with offline support, plus a Capacitor configuration for native shells
- English and French interface
- Light, dark and system themes

### Development & automation

- Docker development and production images
- `clang-format`, `cppcheck`, Prettier and `pre-commit`
- Doxygen and TypeDoc, published to the GitHub Wiki
- GitHub Actions, CodeQL, sanitizers and security scans
- Conventional Commits
- Automated releases with semantic-release

---

# 🏗️ Architecture

The project is deliberately split into layers:

```text
┌─────────────────────────────────────┐
│             React / Vite             │
│   Library • Controls • Player UI     │
└─────────────────┬───────────────────┘
                   │
                   │ TypeScript
                   ▼
┌─────────────────────────────────────┐
│          Emulator Session            │
│                                      │
│  Canvas • Audio • Input • Timing     │
│  Battery saves • Save states         │
└─────────────────┬───────────────────┘
                   │
                   │ WebAssembly
                   ▼
┌─────────────────────────────────────┐
│        EmulatorWrapper (Embind)      │
│        src/wasm/bindings.cc          │
└─────────────────┬───────────────────┘
                   │
                   ▼
┌─────────────────────────────────────┐
│          Console interface           │
└─────────────────┬───────────────────┘
                   │
      ┌───────────┴───────────┐
      │            │           │
      ▼            ▼           ▼            ▼
  ┌───────┐  ┌───────────┐ ┌─────────┐ ┌───────┐
  │CHIP-8 │  │ Atari2600 │ │Game Boy │ │  NES  │
  └───────┘  └───────────┘ └─────────┘ └───────┘
```

The important design principle is that the **C++ emulator does not depend on the web application**.

The browser is only one possible frontend for the emulator core.

---

## 🧩 Emulator abstraction

Every system implements the `Console` interface (`src/emulators/console/Console.hh`). It is deliberately small, so the frontend never needs to know which concrete emulator it is talking to:

| Method | Purpose |
| --- | --- |
| `loadProgram(data, size)` | Load a ROM or cartridge image; returns `false` if it is invalid or unsupported |
| `reset()` | Restart the loaded program |
| `stepFrame()` | Run the machine for one video frame |
| `isHalted()` | Whether the machine has stopped |
| `setKey(key, pressed)` | Forward an input (key identifiers are defined per system) |
| `width()` / `height()` / `framebuffer()` | Size and pixels of the last frame |
| `setAudioSampleRate(hz)` | Sample rate to generate audio for |
| `audioFrameCount()` / `audioSamples()` | Interleaved stereo samples produced by the last frame |
| `saveState()` / `loadState(json)` | Full machine snapshot as JSON |
| `hasBattery()` / `saveRam()` / `loadRam(data, size)` / `consumeRamDirty()` | Battery-backed cartridge RAM (the in-game save) and a flag telling the frontend the game has just written to it |

This makes adding a new system primarily an **emulator-core problem**, rather than a frontend rewrite.

---

# 🔩 Hardware-oriented design

The larger console emulators are decomposed into hardware components instead of implementing the entire machine in a single class.

For example, the Game Boy architecture is roughly:

```text
                  ┌─────────────────┐
                  │    Game Boy     │
                  │     Console     │
                  └────────┬────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │   GameBoyBus    │
                  └────────┬────────┘
                           │
   ┌───────────┬───────────┼───────────┬───────────┐
   ▼           ▼           ▼           ▼           ▼
┌────────┐ ┌────────┐ ┌────────┐ ┌────────┐ ┌───────────┐
│LR35902 │ │  PPU   │ │ Timer  │ │  APU   │ │ Interrupt  │
│  CPU   │ │        │ │        │ │        │ │ controller │
└────────┘ └────────┘ └────────┘ └────────┘ └───────────┘
                    │
          ┌───────────┴───────────┐
          ▼                      ▼
     ┌─────────┐          ┌─────────────┐
     │ Joypad  │          │  Cartridge   │
     │         │          │ (MBC, RAM)   │
     └─────────┘          └─────────────┘
```

The same approach is used for the Atari 2600:

```text
             ┌──────────────────┐
             │   Atari 2600     │
             │     Console      │
             └────────┬─────────┘
                      │
                      ▼
             ┌──────────────────┐
             │   Atari2600Bus   │
             └────────┬─────────┘
                      │
       ┌──────────────┼───────────────┐
       ▼              ▼               ▼
   ┌────────┐    ┌─────────┐    ┌───────────┐
   │ CPU65  │    │  TIA1A  │    │  MOS6532  │
   │ 6507   │    │  Video  │    │   RIOT    │
   └────────┘    └─────────┘    └───────────┘
                                      │
                                      ▼
                                ┌───────────┐
                                │ Cartridge │
                                └───────────┘
```

and for the NES, which reuses the `CPU65` core:

```text
             ┌──────────────────┐
             │       NES        │
             │     Console      │
             └────────┬─────────┘
                      │
                      ▼
             ┌──────────────────┐
             │     NesBus       │
             └────────┬─────────┘
                      │
       ┌──────────────┼───────────────┐
       ▼              ▼               ▼
   ┌────────┐    ┌─────────┐    ┌───────────┐
   │ CPU65  │    │ PPU2C02 │    │  NesAPU   │
   │ 2A03   │    │  Video  │    │  5 chans  │
   └────────┘    └─────────┘    └───────────┘
                                      │
                                      ▼
                                ┌─────────────────┐
                                │ Cartridge + mapper │
                                │ (Mapper000, 001...)│
                                └──────────────────┘
```

This separation makes hardware components reusable, testable and easier to reason about.

---

# 🌐 WebAssembly interface

The browser communicates with the C++ emulator through a small Embind API (the `Emulator` class of `src/wasm/bindings.cc`, typed on the TypeScript side in `web/src/emulator/wasm.ts`).

```text
load(system, rom)  ── returns false if the ROM is rejected; the previous machine is kept
   │
   ├── setSampleRate / reset / stepFrame / setKey / isHalted
   ├── width / height / framebufferPtr       → pixels in WebAssembly memory
   ├── audioFrameCount / audioPtr             → samples in WebAssembly memory
   ├── saveState / loadState                  → JSON snapshot
   └── hasBattery / saveRam / loadRam / consumeRamDirty   → in-game save
```

`framebufferPtr()` and `audioPtr()` are byte addresses in WebAssembly memory: the frontend reads them through typed-array views over `HEAPU8` / `HEAPF32` instead of copying through the binding layer. ROMs are copied in with a single `TypedArray.set()`.

The C++ implementation remains unaware of React, Vite or browser APIs.

---

# 🖥️ Frontend

The browser application lives in `web/`.

```text
web/src/
├── components/   UI building blocks (add-game form, game cards, touch controls, layout, update prompt...)
├── emulator/     EmulatorSession, system definitions, WebAssembly loader, gamepad mappings
├── hooks/        useGames, useSettings, useProfile...
├── i18n/         English / French messages and the I18nProvider
├── lib/          IndexedDB stores (ROMs, games, battery saves), Supabase client, LocalizedError
├── pages/        Library, Player, Controls, Login, Reset password
└── types.ts
```

The frontend is responsible for browser-specific concerns:

- WebAssembly module loading
- Canvas rendering and display options
- Keyboard, gamepad and touch input
- Audio playback and emulation pacing
- Emulator session lifecycle
- The game library and save management
- Accounts, settings and the user interface

The emulator implementation itself stays in C++.

## Where data lives

| Data | Stored in |
| --- | --- |
| ROMs, game metadata, in-game (battery) saves | The browser's **IndexedDB**. Nothing is uploaded: another browser has its own library. |
| Save states | Downloaded and loaded as files |
| Account, profile, key bindings and display settings | **Supabase** (schema and row-level security policies in `web/supabase/schema.sql`) |
| Language, theme | `localStorage` |

Errors raised outside React (the emulator session, the storage helpers, the WebAssembly loader) are `LocalizedError`s: their message is already written in the interface language. Add new user-facing strings to both dictionaries in `web/src/i18n/I18nProvider.tsx`.

---

# 📁 Project structure

```text
emulators-collection/
│
├── .github/
│   ├── workflows/
│   │   ├── checks.yml
│   │   ├── security.yml
│   │   ├── documentation.yml
│   │   ├── release.yml
│   │   ├── labeler.yml
│   │   └── stale.yml
│   └── dependabot.yml
│
├── docker/
│   └── nginx.conf            # production image
│
├── scripts/                  # run.sh (Docker launcher), wiki generation, Prettier hook
│
├── src/
│   ├── wasm/
│   │   └── bindings.cc
│   │
│   └── emulators/
│       ├── console/
│       │   ├── Console.hh        # common interface
│       │   ├── chip8/
│       │   ├── atari2600/
│       │   ├── game_boy/
│       │   └── nes/
│       │
│       ├── cpu/
│       ├── bus/
│       ├── ppu/
│       ├── processor/
│       ├── timer/
│       ├── audio/
│       ├── cartridge/
│       ├── controller/
│       └── interrupt_controller/
│
├── tests/                    # test ROM runners and CTest registration
│
├── web/
│   ├── public/
│   ├── src/                  # see Frontend
│   └── supabase/
│       └── schema.sql
│
├── CMakeLists.txt
├── Dockerfile
├── Doxyfile
├── .clang-format
├── .pre-commit-config.yaml
├── .releaserc.json
├── LICENSE
└── README.md
```

The WebAssembly build output (`emulators.js` and `emulators.wasm`) is generated, not committed: it goes to `web/src/wasm/`.

---

# 🚀 Getting started

## 🐳 Docker

Docker is the recommended way to build the WebAssembly modules and run the web application in a reproducible environment.

### Requirements

- Docker with **BuildKit** enabled
- Docker Buildx (recommended)
- A modern web browser

The Dockerfile uses BuildKit cache mounts to speed up dependency installation. A regular legacy `docker build` without BuildKit will fail on the `RUN --mount` instruction.

### Quick start script

`scripts/run.sh` builds the image, starts the container and **removes the container as soon as it stops** (Ctrl+C, error or normal exit), so nothing is left behind:

```bash
./scripts/run.sh                 # development server (hot reload) on http://localhost:5173
./scripts/run.sh --open          # same, and open the browser once the server is ready
./scripts/run.sh --port 3000     # publish on another host port
./scripts/run.sh prod            # production image (nginx) on http://localhost:8080
./scripts/run.sh --remove-image  # also delete the Docker image on exit
```

If the script is not executable yet, run `chmod +x scripts/run.sh` once (or use `bash scripts/run.sh`).

| Option | Effect |
| --- | --- |
| `dev` (default) / `prod` | Development server on port 5173, or the nginx production image on port 8080 |
| `-p`, `--port PORT` | Host port to publish the app on |
| `-o`, `--open` | Open the app in the default browser once it answers |
| `--remove-image` | Remove the image on exit as well. By default it is kept so the next start is fast |
| `-h`, `--help` | Show the help |

The `prod` mode needs `VITE_SUPABASE_URL` and `VITE_SUPABASE_ANON_KEY`, read from the environment or from `web/.env.local` (see [Run the frontend](#run-the-frontend)). The app is published on all network interfaces, so you can also try it from a phone on the same network at `http://<your-computer-ip>:5173`.

The commands below do the same thing by hand.

### Development image

A plain build produces the development image (it is the last stage of the Dockerfile):

```bash
docker buildx build --load -t emulator-collection .
docker run --rm -p 5173:5173 emulator-collection
```

Then open `http://localhost:5173`.

If you do not have a Buildx builder configured yet, create one first:

```bash
docker buildx create --name local-builder --use
docker buildx inspect --bootstrap
```

Or, with a Docker installation that supports BuildKit through the classic build command:

```bash
DOCKER_BUILDKIT=1 docker build -t emulator-collection .
```

### Production image

The `production` target builds the static site and serves it with an unprivileged nginx, without Node, npm packages or the Emscripten SDK in the final image. Vite inlines the Supabase settings at build time, so they are required build arguments (the anon key is a public client key):

```bash
docker buildx build --load --target production \
    --build-arg VITE_SUPABASE_URL=https://<project>.supabase.co \
    --build-arg VITE_SUPABASE_ANON_KEY=<anon-key> \
    -t emulators-collection:prod .

docker run --rm -p 8080:8080 emulators-collection:prod
```

Then open `http://localhost:8080`. Set `--build-arg VITE_BASE=/sub/path/` to serve the app from a sub-path. nginx falls back to `index.html` for client-side routes, caches the content-hashed assets for a year and never caches `index.html` or the PWA service worker (`docker/nginx.conf`).

### Build stages

1. **`wasm-builder`**: Emscripten compiles the C++ emulator cores into `.js` and `.wasm` artifacts.
2. **`web-base`**: Node.js 24 (Bookworm slim), the npm dependencies, the web sources and the WebAssembly output.
3. **`site-builder`**: `npm run build`, producing the static site.
4. **`production`**: nginx serving the static site.
5. **`dev`** (last, hence the default): the Vite development server on top of `web-base`.

To enable warnings-as-errors in the C++ WebAssembly build:

```bash
docker buildx build --load \
    --build-arg WARNINGS_AS_ERRORS=ON \
    -t emulator-collection .
```

---

## 🛠️ Local development

### Requirements

- C++20 compiler
- CMake ≥ 3.20
- Ninja
- Emscripten
- Node.js 24
- npm

### Build the WebAssembly cores

```bash
emcmake cmake -S . -B build-wasm -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm

mkdir -p web/src/wasm
cp build-wasm/emulators.js build-wasm/emulators.wasm web/src/wasm/
```

### Run the frontend

The frontend reads its Supabase settings from `web/.env.local`:

```text
VITE_SUPABASE_URL=https://<project>.supabase.co
VITE_SUPABASE_ANON_KEY=<anon-key>
```

The database schema and its row-level security policies are in `web/supabase/schema.sql`. `VITE_BASE` sets the base path the site is served from (default `/`).

```bash
cd web
npm ci
npm run dev
```

---

# ⚙️ C++ build configuration

The project targets **C++20** with compiler extensions disabled and strict warnings enabled:

```text
-Wall -Wextra -pedantic -Wold-style-cast
```

`-Werror` is opt-in with `-DWARNINGS_AS_ERRORS=ON` (the Docker build in CI enables it). The WebAssembly target is additionally built with `-O3` and link-time optimisation.

The project also uses `clang-format` and `cppcheck` to enforce consistent code quality.

---

# 🧪 Tests

The emulator cores are checked with community test ROMs (blargg's suites, nestest, MMC3 tests, CHIP-8 test ROMs, cgb-acid2 for the Game Boy Color) and by synthetic Game Boy Color feature ROMs driven by small native runners. The ROMs are downloaded, not committed:

```bash
sh tests/fetch_roms.sh build/test-roms
cmake -S . -B build -G Ninja -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build -LE known-failure --output-on-failure
```

Tests labelled `known-failure` document current accuracy limitations and are expected to fail; run them with `ctest -L known-failure`. When you fix one, remove its label so it becomes a regression guard. See [`tests/README.md`](tests/README.md) for the details.

---

# 🧪 Development checks

Install pre-commit:

```bash
pip install pre-commit
```

Install the Git hooks:

```bash
pre-commit install
```

Run all checks manually:

```bash
pre-commit run --all-files
```

The checks include the repository's configured formatting (`clang-format`, Prettier), static analysis (`cppcheck`) and the Conventional Commits message check.

---

# 🔄 Continuous Integration

The repository uses GitHub Actions for build validation, security analysis, documentation and releases. Dependabot keeps the GitHub Actions and the Docker base images up to date.

## 🧪 `checks.yml`

The main CI pipeline, run for pushes to `main` and pull requests targeting `main`:

- **Commit messages**: every commit of a pull request must follow Conventional Commits
- **Pre-commit**: formatting and static analysis
- **Build**: Docker image build (with warnings as errors) and production build of the website
- **Deploy**: on `main`, the site is published to GitHub Pages

The Docker build uses **Docker Buildx** with GitHub Actions cache, and the pre-commit job caches its Python dependencies.

---

## 🔐 `security.yml`

The security pipeline runs independently from the normal build pipeline: CodeQL analysis, an AddressSanitizer / UndefinedBehaviorSanitizer build, and scans of the repository, the container image and the workflows themselves. Keeping security separate makes failures easier to identify.

---

## 📚 `documentation.yml`

On pushes to `main` that touch the sources, the API documentation is regenerated (Doxygen for C++, TypeDoc for the frontend) and published to the repository's **GitHub Wiki**.

---

## 📦 `release.yml`

Releases are automated with **semantic-release**, which is the only release tooling (see `.releaserc.json`).

On every push to `main` it:

1. Reads Conventional Commits
2. Determines the next version
3. Generates release notes
4. Creates the GitHub release and tag

Examples:

```text
feat(gameboy): add MBC5 support
fix(gameboy): correct interrupt handling
fix(atari2600): fix TIA timing
refactor: reorganize emulator architecture
docs: update architecture documentation
```

---

# 📚 Documentation

The C++ codebase uses **Doxygen** and the frontend uses **TypeDoc**. Both are published to the GitHub Wiki by `documentation.yml`.

Generate the C++ documentation locally with:

```bash
doxygen Doxyfile
```

For diagrams, install Graphviz as well:

```bash
sudo apt install doxygen graphviz
```

The frontend documentation is generated with `npm run docs` from `web/`.

---

# ➕ Adding a new emulator

A new emulator should integrate with the existing architecture instead of introducing a separate frontend implementation.

### 1. Create the system

```text
src/emulators/console/<system>/
```

### 2. Implement `Console`

Implement the interface of `src/emulators/console/Console.hh`: program loading, reset, frame execution, input, framebuffer, audio, display dimensions, save states and, if the hardware has a battery, the battery RAM methods.

### 3. Add hardware components

Depending on the system:

```text
src/emulators/cpu/
src/emulators/bus/
src/emulators/ppu/
src/emulators/timer/
src/emulators/audio/
src/emulators/cartridge/
src/emulators/controller/
src/emulators/processor/
src/emulators/interrupt_controller/
```

### 4. Add the CMake target

Register the new library in `CMakeLists.txt` and link it to the `emulators` WebAssembly target.

### 5. Add WebAssembly bindings

Add the system identifier to `EmulatorWrapper::load()` in `src/wasm/bindings.cc`.

### 6. Add the frontend system definition

Register in `web/src/emulator/`:

- system name and identifier
- ROM extensions
- display properties
- input actions and default key / gamepad mappings

and add its texts to both dictionaries in `web/src/i18n/I18nProvider.tsx`.

### 7. Document it

Add Doxygen documentation for the new components.

### 8. Add tests

Register test ROMs in `tests/RomTests.cmake` whenever a suitable ROM exists.

### 9. Update this README

Add the system to the supported-systems table.

---

# 🧭 Design principles

The project follows a few core principles.

### Separation of concerns

```text
Emulation ≠ Web UI
```

The emulator should not know that it is running in a browser.

### Hardware-oriented composition

Complex systems are built from smaller components:

```text
Console
 ├── Bus
 ├── CPU
 ├── PPU
 ├── Timer
 ├── Input
 ├── Audio
 └── Cartridge
```

### Reusability

Hardware components should be reusable whenever different systems share compatible behaviour (the 6502 core already serves two systems).

### Platform independence

The core should remain independent from:

- React
- TypeScript
- Vite
- DOM APIs
- Canvas APIs
- Web Audio APIs

### Browser as a frontend

WebAssembly is an integration layer, not the emulator itself.

---

# 🤝 Contributing

Contributions are welcome.

Before opening a pull request:

```bash
pre-commit run --all-files
```

Please use **Conventional Commits**: every commit of a pull request is checked, and semantic-release derives the next version from them.

Examples:

```text
feat(gameboy): add MBC3 support
fix(gameboy): fix timer overflow
fix(atari2600): correct TIA rendering
test(chip8): add opcode coverage
refactor: simplify emulator session
docs: update architecture documentation
```

For larger changes, it is recommended to open an issue first to discuss the architecture and implementation.

---

# ⚖️ ROMs and copyrighted material

This project does **not** distribute commercial ROMs. ROMs you add to the library stay in your own browser and are not uploaded anywhere.

Users are responsible for obtaining and using ROM files legally.

Only use ROMs for which you have the appropriate rights or permission.

---

# 📜 License

This project is licensed under the **MIT License**.

See [`LICENSE`](LICENSE) for the complete license text.

---

## 👤 Author

**Alexis Mialon**

- GitHub: [@Vorpalin](https://github.com/Vorpalin)
- Project: [Emulators Collection](https://github.com/Vorpalin/emulators-collection)
