# Emulators Collection

[![CI](https://github.com/Vorpalin/emulators-collection/actions/workflows/checks.yml/badge.svg)](https://github.com/Vorpalin/emulators-collection/actions/workflows/checks.yml)
[![Security](https://github.com/Vorpalin/emulators-collection/actions/workflows/security.yml/badge.svg)](https://github.com/Vorpalin/emulators-collection/actions/workflows/security.yml)
[![License](https://img.shields.io/github/license/Vorpalin/emulators-collection)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![WebAssembly](https://img.shields.io/badge/WebAssembly-Emscripten-654FF0.svg)](https://webassembly.org/)
[![React](https://img.shields.io/badge/React-TypeScript-61DAFB.svg)](https://react.dev/)
[![Vite](https://img.shields.io/badge/Vite-frontend-646CFF.svg)](https://vite.dev/)

**Emulators Collection** is a modular collection of classic computer and video game system emulators written in **C++20** and compiled to **WebAssembly**.

The emulator cores are independent from the browser and expose a common interface. A **React + TypeScript + Vite** frontend connects these cores to the browser, providing video output, audio, input handling and ROM loading.

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
| **Game Boy** | ✅ | Sharp LR35902, PPU, timer, joypad, cartridges |

The project is designed so that additional systems can be integrated without changing the architecture of the existing emulators.

---

## ✨ Features

### Emulator core

- Modular C++20 architecture
- Common `Emulator` interface
- Independent emulator cores
- Reusable hardware components
- CPU, bus, PPU, timer, controller, cartridge and audio abstractions
- Strict compiler warnings
- CMake-based build system
- Native and WebAssembly-oriented architecture

### WebAssembly

- C++ emulator cores compiled with **Emscripten**
- JavaScript-compatible WebAssembly bindings
- ROM loading from the browser
- Frame execution from JavaScript
- Framebuffer access
- Audio sample access
- Input forwarding
- Emulator dimension queries

### Web application

- React
- TypeScript
- Vite
- Browser-based rendering
- Keyboard input
- Controller-oriented input abstraction
- Browser audio
- Multiple systems exposed through the same interface
- System-specific ROM validation

### Development & automation

- Docker development environment
- `clang-format`
- `cppcheck`
- `pre-commit`
- Doxygen
- GitHub Actions
- Automated security checks
- Conventional Commits
- Automated releases with semantic-release

---

# 🏗️ Architecture

The project is deliberately split into three layers:

```text
┌──────────────────────────────────────┐
│             React / Vite             │
│             Web frontend             │
└──────────────────┬───────────────────┘
                   │
                   │ TypeScript
                   ▼
┌──────────────────────────────────────┐
│          Emulator Session            │
│                                      │
│  Canvas • Audio • Input • Timing     │
└──────────────────┬───────────────────┘
                   │
                   │ WebAssembly
                   ▼
┌──────────────────────────────────────┐
│           WASM bindings              │
│              Emscripten              │
└──────────────────┬───────────────────┘
                   │
                   ▼
┌──────────────────────────────────────┐
│          Emulator interface          │
└──────────────────┬───────────────────┘
                   │
       ┌───────────┼───────────┐
       │           │           │
       ▼           ▼           ▼
   ┌───────┐  ┌─────────┐  ┌─────────┐
   │CHIP-8 │  │Atari2600│  │ Game Boy│
   └───────┘  └─────────┘  └─────────┘
```

The important design principle is that the **C++ emulator does not depend on the web application**.

The browser is only one possible frontend for the emulator core.

---

## 🧩 Emulator abstraction

Complete emulator systems implement a common interface.

Conceptually:

```cpp
class Emulator {
public:
    virtual ~Emulator() = default;

    virtual void reset() = 0;
    virtual void loadRom(...) = 0;
    virtual void runFrame() = 0;

    virtual void setInput(...) = 0;

    virtual const uint8_t* framebuffer() const = 0;
    virtual const float* audioBuffer() const = 0;

    virtual int width() const = 0;
    virtual int height() const = 0;
};
```

The exact implementation is system-specific, but the frontend does not need to know which concrete emulator it is communicating with.

This makes adding a new system primarily an **emulator-core problem**, rather than a frontend rewrite.

---

# 🔩 Hardware-oriented design

The larger console emulators are decomposed into hardware components instead of implementing the entire machine in a single class.

For example, the Game Boy architecture is roughly:

```text
                  ┌─────────────────┐
                  │    Game Boy     │
                  │    Emulator     │
                  └────────┬────────┘
                           │
                           ▼
                  ┌─────────────────┐
                  │   GameBoyBus    │
                  └────────┬────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
   ┌─────────┐        ┌─────────┐        ┌─────────┐
   │ LR35902 │        │   PPU   │        │  Timer  │
   │   CPU   │        │         │        │         │
   └─────────┘        └─────────┘        └─────────┘
        │
        ├───────────────┐
        │               │
        ▼               ▼
   ┌─────────┐      ┌───────────┐
   │ Joypad  │      │ Cartridge │
   └─────────┘      └───────────┘
```

The same approach is used for the Atari 2600:

```text
             ┌──────────────────┐
             │   Atari 2600     │
             │    Emulator      │
             └────────┬─────────┘
                      │
                      ▼
             ┌──────────────────┐
             │   Atari2600Bus   │
             └────────┬─────────┘
                      │
       ┌──────────────┼───────────────┐
       │              │               │
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

This separation makes hardware components reusable, testable and easier to reason about.

---

# 🌐 WebAssembly interface

The browser communicates with the C++ emulator through a small WebAssembly API.

The frontend can:

```text
Load ROM
   │
   ▼
Create emulator
   │
   ├── Reset
   ├── Run frame
   ├── Send input
   ├── Read framebuffer
   ├── Read audio samples
   └── Query dimensions
```

The C++ implementation remains unaware of React, Vite or browser APIs.

This separation allows the same emulator core to potentially be reused by another frontend in the future.

---

# 🖥️ Frontend

The browser application lives in `web/`.

```text
web/
├── public/
│   └── wasm/
├── scripts/
└── src/
    ├── components/
    ├── emulator/
    ├── pages/
    ├── systems/
    └── ...
```

The frontend is responsible for browser-specific concerns:

- WebAssembly module loading
- ROM loading
- Canvas rendering
- Keyboard input
- Controller input
- Audio playback
- Emulator session lifecycle
- System selection
- User interface

The emulator implementation itself stays in C++.

---

# 📁 Project structure

```text
emulators-collection/
│
├── .github/
│   └── workflows/
│       ├── checks.yml
│       ├── security.yml
│       └── release.yml
│
├── assets/
│   └── fonts/
│
├── scripts/
│
├── src/
│   ├── wasm/
│   │   └── bindings.cc
│   │
│   └── emulators/
│       ├── console/
│       │   ├── chip_8/
│       │   ├── atari2600/
│       │   └── gameboy/
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
├── web/
│   ├── public/
│   │   └── wasm/
│   ├── scripts/
│   └── src/
│       ├── components/
│       ├── emulator/
│       ├── pages/
│       ├── systems/
│       └── ...
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

---

# 🚀 Getting started

## 🐳 Docker

Docker is the recommended way to run the complete web application.

### Requirements

- Docker
- A modern web browser

Build the image:

```bash
docker build -t emulator-collection .
```

Run it:

```bash
docker run --rm -p 5173:5173 emulator-collection
```

Then open:

```text
http://localhost:5173
```

The Docker build contains the tooling required to build the emulator cores to WebAssembly and run the frontend.

---

## 🛠️ Local development

### Requirements

- C++20 compiler
- CMake ≥ 3.20
- Ninja
- Emscripten
- Node.js
- npm

### Build the WebAssembly cores

```bash
emcmake cmake \
    -S . \
    -B build-wasm \
    -DCMAKE_BUILD_TYPE=Release
```

Then:

```bash
cmake --build build-wasm
```

The generated WebAssembly artifacts are made available to the frontend under:

```text
web/public/wasm/
```

### Run the frontend

```bash
cd web
npm ci
npm run dev
```

---

# ⚙️ C++ build configuration

The project targets **C++20** with compiler extensions disabled and strict warnings enabled.

The build uses options including:

```text
-O3
-Wall
-Wextra
-Werror
-pedantic
-Wold-style-cast
```

Warnings are treated as errors.

The project also uses `clang-format` and `cppcheck` to enforce consistent code quality.

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

The checks include the repository's configured formatting and static-analysis rules.

---

# 🔄 Continuous Integration

The repository uses GitHub Actions for build validation, security analysis and releases.

```text
                         GitHub Events
                              │
             ┌────────────────┼────────────────┐
             │                │                │
             ▼                ▼                ▼
        checks.yml       security.yml      release.yml
             │                │                │
       ┌─────┼─────┐          │                │
       │     │     │          │                │
       ▼     ▼     ▼          ▼                ▼
   Format  Commit  Build   Security       semantic-
            msg            analysis        release
                                                │
                                                ▼
                                         GitHub Release
```

## 🧪 `checks.yml`

The main CI pipeline validates:

- C/C++ formatting
- Static analysis
- Commit messages
- Docker build
- WebAssembly/frontend build

It runs for pushes to `main` and pull requests targeting `main`.

---

## 🔐 `security.yml`

The security pipeline performs automated security checks independently from the normal build pipeline.

The goal is to detect issues involving:

- Dependencies
- Source code
- GitHub Actions
- Dependency changes
- Other configured security checks

Keeping security separate makes failures easier to identify and maintain.

---

## 📦 `release.yml`

Releases are automated with **semantic-release**.

The release process:

1. Reads Conventional Commits
2. Determines the next version
3. Generates release notes
4. Creates the GitHub release

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

The C++ codebase uses **Doxygen** for API documentation.

Generate the documentation with:

```bash
doxygen Doxyfile
```

For diagrams, install Graphviz as well:

```bash
sudo apt install doxygen graphviz
```

The documentation is intended to describe the emulator architecture, hardware components and public interfaces.

---

# ➕ Adding a new emulator

A new emulator should integrate with the existing architecture instead of introducing a separate frontend implementation.

### 1. Create the system

```text
src/emulators/console/<system>/
```

### 2. Implement `Emulator`

Implement the common emulator lifecycle:

- ROM loading
- Reset
- Frame execution
- Input
- Framebuffer
- Audio
- Display dimensions

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

Register the new implementation in `CMakeLists.txt`.

### 5. Add WebAssembly bindings

Expose the emulator through the WASM interface where necessary.

### 6. Add the frontend system definition

Register:

- system name
- ROM extensions
- display properties
- input mapping
- WASM module

### 7. Document it

Add Doxygen documentation for the new components.

### 8. Add tests

Hardware behaviour should be covered by tests whenever possible.

### 9. Update this README

Add the system to the supported-emulators table.

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

Hardware components should be reusable whenever different systems share compatible behaviour.

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

Please use **Conventional Commits**.

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

This project does **not** distribute commercial ROMs.

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

---

<p align="center">
  Built with C++20, WebAssembly, React and TypeScript.
</p>
