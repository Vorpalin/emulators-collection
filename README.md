# Emulators Collection

A collection of classic video game console and computer emulators written in **C++20**, running in the browser through **WebAssembly** with a **React/Vite** frontend.

The project is designed around a modular architecture where each emulator implements a common `Emulator` interface while reusing dedicated hardware components such as CPUs, buses, PPUs, timers, controllers, audio hardware and cartridges.

The C++ emulator cores remain independent from the web interface and are compiled to WebAssembly using **Emscripten**. The React frontend provides the browser-based user interface.

---

## 🎮 Available Emulators

| Emulator | Status | Description |
| --- | --- | --- |
| **CHIP-8** | ✅ Available | CHIP-8 interpreter |
| **Super-CHIP 48** | ✅ Available | Extended CHIP-8 interpreter with 128×64 display and additional instructions |
| **Atari 2600** | ✅ Available | Atari 2600 emulator with MOS 6507, MOS 6532 RIOT and TIA |
| **Game Boy** | ✅ Available | Game Boy emulator with Sharp LR35902 CPU, PPU, timer, joypad and cartridge support |

More emulators will be added over time.

---

## ✨ Features

- Modular emulator architecture
- Common `Emulator` interface for supported systems
- Reusable hardware components:
  - CPUs
  - buses
  - PPUs
  - timers
  - controllers
  - audio hardware
  - cartridges
- C++20 implementation
- Strict compiler warnings with `-Werror`
- Web-based user interface
- React + TypeScript frontend
- Vite development environment
- WebAssembly compilation with Emscripten
- Browser-based video rendering
- Browser-based audio output
- Keyboard/controller input handling
- Support for multiple emulators within the same web application
- Docker-based WebAssembly and frontend build
- Doxygen API documentation
- Automated releases with semantic-release
- GitHub Actions CI with pre-commit and Docker build checks

---

## 📋 Requirements

### Docker

Docker is the recommended way to build and run the web application.

You need:

- [Docker](https://www.docker.com/)
- A modern web browser

The Docker image contains the tools required to compile the C++ emulator cores to WebAssembly and run the Vite frontend.

### Local development

For WebAssembly development, you need:

- A C++20 compiler
- [CMake](https://cmake.org/) ≥ 3.20
- [Ninja](https://ninja-build.org/) (recommended)
- [Emscripten](https://emscripten.org/)
- Node.js
- npm

---

## 🏗️ Building

### With Docker

Build the Docker image from the project root:

```bash
docker image build -t emulator-collection .
```

Run the web application:

```bash
docker run --rm -p 5173:5173 emulator-collection
```

The application is then available at:

```text
http://localhost:5173
```

The Docker image builds the C++ emulator cores to WebAssembly and starts the Vite development server.

### WebAssembly build

The WebAssembly build can also be performed locally.

The build process uses Emscripten and CMake:

```bash
emcmake cmake \
    -S . \
    -B build-wasm \
    -DCMAKE_BUILD_TYPE=Release

cmake --build build-wasm
```

The generated WebAssembly files are copied to:

```text
web/public/wasm/
```

The web frontend can then load the generated WebAssembly module.

### Frontend

Install the frontend dependencies:

```bash
cd web
npm ci
```

Start the Vite development server:

```bash
npm run dev
```

---

## ⚙️ Build Configuration

The project is compiled as C++20 with compiler extensions disabled and strict warnings enabled.

The current build configuration includes:

```text
-O3
-Wall
-Wextra
-Werror
-pedantic
-Wold-style-cast
```

Any compiler warning therefore causes the build to fail.

The WebAssembly build uses Emscripten to expose the emulator core to JavaScript through WebAssembly bindings.

---

## 🌐 Web Application

The frontend is located in the `web/` directory and is implemented using:

- React
- TypeScript
- Vite

The frontend is responsible for:

- Displaying the emulator interface
- Loading emulator WebAssembly modules
- Loading ROM data
- Forwarding keyboard/controller input
- Displaying the emulator framebuffer
- Playing generated audio
- Managing the current emulator session

The emulator implementation itself remains in C++ and does not depend on React or browser-specific APIs.

### WebAssembly interface

The C++ WebAssembly bindings expose the emulator through a small JavaScript-compatible interface.

The browser can:

- Load a ROM
- Reset the emulator
- Execute frames
- Send input
- Access the framebuffer
- Access generated audio samples
- Query the emulator dimensions

This keeps the emulator core independent from the frontend while allowing it to run directly in the browser.

---

## 🏗️ Project Structure

The project is organized around the C++ emulator cores, WebAssembly bindings and web frontend:

```text
emulators-collection/
├── .github/
│   └── workflows/                     # GitHub Actions CI
├── scripts/                            # Helper and build scripts
├── src/
│   ├── wasm/
│   │   └── bindings.cc                 # WebAssembly bindings
│   │
│   └── emulators/
│       ├── console/                    # Complete emulator systems
│       ├── cpu/                        # CPU implementations
│       ├── bus/                        # System memory buses
│       ├── ppu/                        # Video hardware
│       ├── processor/                  # Additional processors/chips
│       ├── timer/                      # Hardware timers
│       ├── audio/                      # Audio hardware
│       ├── cartridge/                  # Cartridge and ROM handling
│       └── controller/                 # Input/controller hardware
│
├── web/
│   ├── public/
│   │   └── wasm/                       # Generated WebAssembly files
│   ├── scripts/                        # Frontend/build scripts
│   └── src/
│       ├── emulator/                   # WASM loading and emulator sessions
│       ├── pages/                      # Web pages
│       ├── components/                 # React components
│       └── ...
│
├── .clang-format                       # C/C++ formatting rules
├── .pre-commit-config.yaml             # Pre-commit configuration
├── .releaserc.json                     # semantic-release configuration
├── .dockerignore
├── .gitignore
├── CMakeLists.txt                      # C++/WebAssembly build configuration
├── Dockerfile                          # Web application Docker image
├── Doxyfile                            # Doxygen configuration
├── LICENSE                             # MIT license
└── README.md
```

The C++ emulator cores are kept separate from the web application. The `web/` directory contains only the browser-facing application and its WebAssembly integration.

---

## 🧩 Architecture

The application is divided into three main layers:

```text
                         ┌──────────────────────┐
                         │      React/Vite      │
                         │    Web Frontend      │
                         └──────────┬───────────┘
                                    │
                                    ▼
                         ┌──────────────────────┐
                         │    WASM Bindings     │
                         │      Emscripten      │
                         └──────────┬───────────┘
                                    │
                                    ▼
                         ┌──────────────────────┐
                         │   Emulator Interface │
                         └──────────┬───────────┘
                                    │
                  ┌─────────────────┼─────────────────┐
                  │                 │                 │
                  ▼                 ▼                 ▼
             ┌─────────┐       ┌─────────┐       ┌─────────┐
             │ CHIP-8  │       │ Atari   │       │  Game   │
             │         │       │  2600   │       │   Boy   │
             └─────────┘       └─────────┘       └─────────┘
```

The web frontend communicates with the C++ emulator through the WebAssembly bindings.

The emulator implementations do not depend on React, Vite or browser APIs.

### Emulator interface

All complete emulator systems implement the common `Emulator` interface.

The interface provides the operations required by the frontend through the WebAssembly bindings, including:

- Loading a ROM
- Resetting the emulator
- Executing a frame
- Handling input
- Accessing the framebuffer
- Accessing audio samples
- Querying display dimensions

This allows the web application to interact with different systems without depending on their concrete implementation.

### Inside a hardware emulator

Console emulators are composed of smaller hardware components.

For example, the Game Boy is organized around a system bus:

```text
                    ┌──────────────┐
                    │   Game Boy   │
                    │   Emulator   │
                    └──────┬───────┘
                           │
                    ┌──────▼───────┐
                    │  GameBoyBus  │
                    └──────┬───────┘
                           │
         ┌──────────┬──────┼───────┬───────────┐
         │          │      │       │           │
         ▼          ▼      ▼       ▼           ▼
       CPU         PPU   Timer   Joypad    Cartridge
     LR35902    GameBoyPPU
```

The same principle is used for the Atari 2600, where the system bus connects the CPU, cartridge, RIOT and TIA components.

### Hardware components

| System | CPU | Video | Other components |
| --- | --- | --- | --- |
| **Atari 2600** | `CPU65` | `TIA1A` | `MOS6532`, cartridge, `Atari2600Bus` |
| **Game Boy** | `LR35902` | `GameBoyPPU` | `GameBoyTimer`, joypad, cartridge, `GameBoyBus` |

The Game Boy implementation therefore keeps the CPU, video, memory mapping, timing, input, audio and cartridge logic as separate components rather than putting the complete system into a single class.

---

## 🎮 Emulator Sessions

The frontend creates an emulator session around the WebAssembly emulator instance.

Conceptually:

```text
React Page
    │
    ▼
EmulatorSession
    │
    ├── Canvas
    ├── Web Audio
    ├── Keyboard Input
    │
    ▼
WebAssembly Emulator
    │
    ▼
C++ Emulator Core
```

The session is responsible for connecting browser APIs to the platform-independent emulator core.

For example:

- The C++ framebuffer is copied into a browser canvas.
- C++ audio samples are forwarded to the Web Audio API.
- Keyboard events are converted into emulator input.
- The emulator executes frames through the WebAssembly module.

---

## ➕ Adding a New Emulator

To add a new system:

1. Create the console implementation under:

   ```text
   src/emulators/console/<name>/
   ```

2. Implement the common `Emulator` interface.

3. Add the required hardware components under the appropriate directory:

   ```text
   src/emulators/audio/
   src/emulators/cpu/
   src/emulators/bus/
   src/emulators/ppu/
   src/emulators/timer/
   src/emulators/cartridge/
   src/emulators/processor/
   src/emulators/controller/
   src/emulators/interrupt_controller/
   ```

4. Add the corresponding source files and target to `CMakeLists.txt`.

5. Add the emulator to the WebAssembly bindings if required.

6. Add the corresponding system definition to the web frontend.

7. Add Doxygen documentation for new classes.

8. Add the emulator to the table in this README.

---

## 📚 Documentation

The source code is documented with [Doxygen](https://www.doxygen.nl/).

The configuration is stored in:

```text
Doxyfile
```

Install Doxygen and Graphviz:

```bash
sudo apt-get install doxygen graphviz
```

Generate the documentation with:

```bash
doxygen Doxyfile
```

The generated HTML documentation can then be opened in a browser.

---

## 🔄 Continuous Integration & Automation

The project uses **GitHub Actions** to automatically validate, secure and release the project.

The workflows are located in:

```text
.github/workflows/
├── checks.yml
├── security.yml
└── release.yml
```

### 🧪 CI — `checks.yml`

The main CI workflow verifies that changes are correctly formatted, validated and build successfully.

It runs on:

- Pushes to `main`
- Pull requests targeting `main`

The workflow performs the following checks:

- **Pre-commit**
  - C/C++ formatting with `clang-format`
  - Static analysis with `cppcheck`
  - Other repository checks configured in `.pre-commit-config.yaml`
- **Commit message**
  - Validates commit messages with the `commit-msg` check
  - Ensures commits follow the project's expected commit message format
- **Build**
  - Builds the WebAssembly/web application using the project's `Dockerfile`

The same pre-commit checks can be run locally with:

```bash
pre-commit run --all-files
```

Commit messages should follow the **Conventional Commits** format, for example:

```text
feat(gameboy): add MBC3 support
fix(atari2600): correct TIA timing
refactor: reorganize emulator architecture
docs: update README
```

### 🔐 Security — `security.yml`

The security workflow performs automated security checks on the repository.

It includes the project's configured security analysis and dependency checks.

This workflow helps detect:

- Vulnerabilities in dependencies
- Security issues in the source code
- Problems introduced by dependency or workflow changes

Security checks are kept separate from the normal CI pipeline so that code quality, build validation and security analysis remain independently visible.

### 📦 Release — `release.yml`

The release workflow automatically creates releases from the `main` branch.

It uses **semantic-release** to:

1. Analyse commit messages
2. Determine the appropriate version
3. Generate release notes
4. Create the corresponding GitHub release

This relies on **Conventional Commits**.

Examples:

```text
feat(gameboy): add MBC5 cartridge support
fix(gameboy): fix interrupt handling
docs: update architecture documentation
```

### 🔗 Workflow overview

```text
                    ┌─────────────────────┐
                    │   GitHub Events     │
                    └──────────┬──────────┘
                               │
                ┌──────────────┼──────────────┐
                │              │              │
                ▼              ▼              ▼
          ┌──────────┐   ┌───────────┐  ┌───────────┐
          │checks.yml│   │security.yml│ │release.yml│
          └────┬─────┘   └─────┬─────┘  └─────┬─────┘
               │               │              │
        ┌──────┼──────┐        │              │
        ▼      ▼      ▼        ▼              ▼
    Pre-commit Commit  Build  Security     Semantic
              message         checks       release
                                             │
                                             ▼
                                       GitHub Release
```

| Workflow | Purpose |
| --- | --- |
| `checks.yml` | Code quality, commit-message validation and WebAssembly/web build |
| `security.yml` | Security and dependency analysis |
| `release.yml` | Automated versioning and GitHub releases |

---

## 🛠️ Development

### Pre-commit

The project uses **pre-commit** to automatically check the code before commits.

Install it with:

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

The configuration is located at:

```text
.pre-commit-config.yaml
```

C/C++ formatting rules are defined in:

```text
.clang-format
```

### Code style

The project follows these main rules:

- C++20
- Compiler extensions disabled
- `-Wall`
- `-Wextra`
- `-Werror`
- `-pedantic`
- `-Wold-style-cast`
- `clang-format` for formatting
- `cppcheck` for static analysis

Use C++ casts such as:

```cpp
static_cast<int>(value)
```

instead of C-style casts.

---

## 📦 Releases

Releases are automated with [semantic-release](https://semantic-release.gitbook.io/).

The configuration is located in:

```text
.releaserc.json
```

Conventional Commits are recommended:

```text
feat(gameboy): add MBC3 real-time clock
fix(atari2600): correct TIA sprite positioning
refactor: reorganize emulator architecture
docs: update README
```

---

## 📄 License

This project is licensed under the **MIT License**.

See the [LICENSE](LICENSE) file for more information.
