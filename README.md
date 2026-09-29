# Emulators Collection

A collection of classic video game console and computer emulators written in **C++20**, unified behind a single SDL2-based application.

The project is designed around a modular architecture where each emulator implements a common `Emulator` interface while reusing dedicated hardware components such as CPUs, buses, PPUs, timers, controllers and cartridges.

## 🎮 Available Emulators

| Emulator          | Status            | Description                                                                        |
| ----------------- | ----------------- | ---------------------------------------------------------------------------------- |
| **CHIP-8**        | ✅ Available       | CHIP-8 interpreter                                                                 |
| **Super-CHIP 48** | ✅ Available       | Extended CHIP-8 interpreter with 128×64 display and additional instructions        |
| **Atari 2600**    | ✅ Available       | Atari 2600 emulator with MOS 6507, MOS 6532 RIOT and TIA                           |
| **Game Boy**      | 🚧 In development | Game Boy emulator with Sharp LR35902 CPU, PPU, timer, joypad and cartridge support |

More emulators will be added over time.

## ✨ Features

* Modular emulator architecture
* Common `Emulator` interface for supported systems
* Reusable hardware components:

  * CPUs
  * buses
  * PPUs
  * timers
  * controllers
  * cartridges
* C++20 implementation
* Strict compiler warnings with `-Werror`
* Docker-based development and execution
* Game/ROM selection system
* SDL2-based graphical interface
* SDL2_ttf text rendering
* Audio support for systems that provide audio hardware
* Support for multiple emulators within the same application
* Doxygen API documentation
* Automated releases with semantic-release
* GitHub Actions CI with pre-commit and Docker build checks

## 📋 Requirements

### With Docker

Docker is the recommended way to build and run the project.

You need:

* [Docker](https://www.docker.com/)
* Linux or **WSL2 with WSLg** for graphical output
* A compatible game/ROM collection

### Without Docker

For a native build, you need:

* A C++20 compiler such as GCC or Clang
* [CMake](https://cmake.org/) ≥ 3.20
* [Ninja](https://ninja-build.org/) (optional)
* SDL2 development libraries
* SDL2_ttf development libraries

On Debian/Ubuntu:

```bash
sudo apt-get install build-essential cmake ninja-build libsdl2-dev libsdl2-ttf-dev
```

> **Note:** ROMs are not included in this repository. Only use ROMs that you legally own or have permission to use.

## 🏗️ Building

### With Docker

Build the Docker image from the project root:

```bash
docker build -t emulator-collection .
```

The image uses Ubuntu and builds the project with CMake.

The resulting executable is:

```text
build/emulators-collection
```

### Native build

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/emulators-collection
```

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

## 🎮 Adding Games

Create the `games` directory if it does not already exist:

```bash
mkdir -p games
```

Then place your compatible ROMs inside it:

```text
games/
├── game1.ch8      # CHIP-8 / Super-CHIP
├── game2.bin      # Atari 2600
├── game3.gb       # Game Boy
└── ...
```

The game selector scans the `games` directory and creates a `Game` object for each ROM.

The appropriate emulator is selected according to the ROM file type.

## ▶️ Running

### WSL2 + WSLg

When running through **WSL2**, the application can use WSLg for graphical output.

Run the Docker container with:

```bash
docker run --rm -it \
  --user "$(id -u):$(id -g)" \
  -e DISPLAY=$DISPLAY \
  -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
  -e XDG_RUNTIME_DIR=/tmp/runtime \
  -e PULSE_SERVER=unix:/tmp/runtime/pulse/native \
  -e SDL_AUDIODRIVER=pulse \
  -v /mnt/wslg/runtime-dir:/tmp/runtime \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "$(pwd)/games:/app/games" \
  emulator-collection
```

The local `games` directory is mounted into the container at `/app/games`.

This allows ROMs added on the host to be immediately available to the application.

### Native Linux

After a native build, run the application from the project root:

```bash
./build/emulators-collection
```

The application expects the `games` and `assets` directories to be available from the current working directory.

## 🏗️ Project Structure

The project is organized around the application, game management and emulator hardware layers:

```text
emulators-collection/
├── .github/
│   └── workflows/
│       └── checks.yml                 # GitHub Actions CI
├── assets/
│   └── fonts/                         # UI fonts
├── scripts/                           # Helper scripts
├── src/
│   ├── main.cc                        # Application entry point
│   │
│   ├── ui/                            # SDL2 graphical interface
│   │   ├── UI.cc
│   │   └── UI.hh
│   │
│   ├── game/                          # ROM/game representation
│   │   ├── Game.cc
│   │   └── Game.hh
│   │
│   ├── game_selector/                 # ROM discovery and selection
│   │   ├── GameSelector.cc
│   │   └── GameSelector.hh
│   │
│   └── emulators/
│       ├── console/                   # Complete emulator systems
│       │   ├── chip8/
│       │   ├── atari2600/
│       │   └── game_boy/
│       │
│       ├── cpu/                       # CPU implementations
│       │   ├── CPU65/
│       │   └── LR35902/
│       │
│       ├── bus/                       # System memory buses
│       │   ├── atari2600_bus/
│       │   └── game_boy_bus/
│       │
│       ├── ppu/                       # Video hardware
│       │   ├── TIA1A/
│       │   └── game_boy_ppu/
│       │
│       ├── processor/                 # Additional processors/chips
│       │   └── MOS6532/
│       │
│       ├── timer/                     # Hardware timers
│       │   └── game_boy_timer/
│       │
│       └── cartridge/                 # Cartridge and ROM handling
│           ├── Cartridge.cc
│           └── game_boy_cartridge/
│
├── games/                             # Local ROM collection
├── .clang-format                      # C/C++ formatting rules
├── .pre-commit-config.yaml            # Pre-commit configuration
├── .releaserc.json                    # semantic-release configuration
├── .dockerignore
├── .gitignore
├── CMakeLists.txt                     # Build configuration
├── Dockerfile                          # Docker build configuration
├── Doxyfile                            # Doxygen configuration
├── LICENSE                             # MIT license
└── README.md
```

The current CMake configuration reflects this organization, with separate source files for CPUs, buses, PPUs, timers, cartridges, consoles, game management and UI.

## 🧩 Architecture

The application is structured around a small number of clearly separated responsibilities.

```text
                         ┌─────────────────┐
                         │       UI        │
                         │     SDL2        │
                         └────────┬────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │  GameSelector   │
                         │ ROM discovery   │
                         └────────┬────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │      Game       │
                         │ ROM + Emulator  │
                         └────────┬────────┘
                                  │
                                  ▼
                         ┌─────────────────┐
                         │    Emulator     │
                         │   Interface     │
                         └────────┬────────┘
                                  │
                    ┌─────────────┼─────────────┐
                    │             │             │
                    ▼             ▼             ▼
                ┌────────┐   ┌─────────┐   ┌─────────┐
                │ CHIP-8 │   │ Atari   │   │  Game   │
                │        │   │  2600   │   │   Boy   │
                └────────┘   └─────────┘   └─────────┘
```

The application entry point is intentionally small: `main.cc` creates the `UI` and starts its main loop.

### Game management

`GameSelector` is responsible for discovering ROMs in the `games` directory, maintaining the current selection and launching the selected game.

```text
UI
 │
 ▼
GameSelector
 │
 ├── Game
 ├── Game
 └── Game
      │
      ▼
   Emulator
```

Each `Game` associates a ROM file with the emulator implementation required to execute it.

### Emulator interface

All complete emulator systems implement the common `Emulator` interface.

The interface provides the operations required by the application:

```cpp
class Emulator {
public:
    virtual ~Emulator() = default;

    virtual void loadProgram(const std::string& filename) = 0;
    virtual int run() = 0;
    virtual void setRenderer(SDL_Renderer* renderer) = 0;
};
```

This allows `Game` and `GameSelector` to interact with different systems without depending on their concrete implementation.

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
        ┌──────────┬───────┼────────┬───────────┐
        │          │       │        │           │
        ▼          ▼       ▼        ▼           ▼
      CPU         PPU     Timer    Joypad    Cartridge
   LR35902    GameBoyPPU
```

The same principle is used for the Atari 2600, where the system bus connects the CPU, cartridge, RIOT and TIA components.

### Hardware components

| System         | CPU       | Video        | Other components                                    |
| -------------- | --------- | ------------ | --------------------------------------------------- |
| **Atari 2600** | `CPU65`   | `TIA1A`      | `MOS6532`, cartridge, `Atari2600Bus`                |
| **Game Boy**   | `LR35902` | `GameBoyPPU` | `GameBoyTimer`, controller, cartridge, `GameBoyBus` |

The Game Boy implementation therefore keeps the CPU, video, memory mapping, timing, input and cartridge logic as separate components rather than putting the complete system into a single class.

## ➕ Adding a New Emulator

To add a new system:

1. Create the console implementation under:

   ```text
   src/emulators/console/<name>/
   ```

2. Implement the common `Emulator` interface.

3. Add the required hardware components under the appropriate directory:

   ```text
   src/emulators/cpu/
   src/emulators/bus/
   src/emulators/ppu/
   src/emulators/timer/
   src/emulators/cartridge/
   src/emulators/processor/
   ```

4. Add the corresponding source files to `CMakeLists.txt`.

5. Connect the emulator to the game-selection system so its ROM format can be recognized.

6. Add Doxygen documentation for new classes.

7. Add the emulator to the table in this README.

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

## 🔄 Continuous Integration

The project uses **GitHub Actions** to automatically verify changes.

The CI workflow runs on:

* Pushes to `main`
* Pull requests targeting `main`

The workflow currently performs three main checks.

### Commit messages

Pull requests have their commit messages checked using the project's pre-commit configuration.

### Pre-commit

The complete pre-commit configuration is executed:

```bash
pre-commit run --all-files
```

This includes checks such as:

* `clang-format`
* `cppcheck`
* Repository-wide validation

### Build

After the pre-commit checks succeed, GitHub Actions builds the Docker image using Docker Buildx.

The workflow runs on **Ubuntu 24.04** and pins its GitHub Actions dependencies to commit SHAs.

The workflow is defined in:

```text
.github/workflows/checks.yml
```

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

* C++20
* Compiler extensions disabled
* `-Wall`
* `-Wextra`
* `-Werror`
* `-pedantic`
* `-Wold-style-cast`
* `clang-format` for formatting
* `cppcheck` for static analysis

Use C++ casts such as:

```cpp
static_cast<int>(value)
```

instead of C-style casts.

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

## 🗺️ Roadmap

Current development focuses on:

* Improving emulator accuracy
* Implementing missing CPU instructions
* Improving graphics and input systems
* Completing the Game Boy emulator
* Improving PPU accuracy
* Improving cartridge and mapper support
* Expanding ROM/game support
* Improving the emulator architecture
* Adding additional console and computer emulators
* Improving testing and validation

## 🤝 Contributing

Contributions are welcome.

1. Fork the repository and create a feature branch.

2. Make your changes while following the project architecture and coding style.

3. Run:

   ```bash
   pre-commit run --all-files
   ```

4. Make sure the Docker build succeeds.

5. Open a pull request targeting `main`.

When adding hardware, prefer creating a dedicated reusable component rather than putting the implementation directly into a console class.

## 📄 License

This project is licensed under the **MIT License**.

See the [LICENSE](LICENSE) file for more information.
