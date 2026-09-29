# Emulators Collection

A collection of classic video game console and computer emulators written in **C++20**, all unified behind a single SDL-based interface.

The project is designed with a modular architecture so that different emulators can share common components such as game management, input handling, and the user interface.

## 🎮 Available Emulators

| Emulator           | Status            | Description                                                                                                        |
| ------------------ | ----------------- | ------------------------------------------------------------------------------------------------------------------ |
| **CHIP-8**         | ✅ Available      | CHIP-8 interpreter                                                                                                 |
| **Super-CHIP 48**  | ✅ Available      | Extended CHIP-8 interpreter with 128×64 display and additional instructions                                        |
| **Atari 2600**     | ✅ Available      | Atari 2600 emulator (MOS 6507 CPU, MOS 6532 RIOT, TIA video)                                                       |
| **Game Boy**       | 🚧 In development | Game Boy emulator (Sharp LR35902 CPU, PPU, timer, joypad, MBC1/MBC2/MBC3/MBC5 cartridges)                          |

More emulators will be added over time.

## ✨ Features

* Modular emulator architecture, with reusable building blocks (CPU, bus, PPU, cartridge, timer...)
* C++20 implementation, compiled with strict warnings (`-Wall -Wextra -Wold-style-cast -pedantic -Werror`)
* Docker-based development and execution
* Game/ROM selection system
* SDL2-based graphical interface (`SDL2_ttf` for text rendering)
* Audio output through PulseAudio (in the Docker setup)
* Support for multiple emulators within the same application
* Designed to be easily extended with new systems
* API documentation generated with Doxygen
* Automated releases with semantic-release

## 📋 Requirements

### With Docker (recommended)

* [Docker](https://www.docker.com/)
* Linux or **WSL2 with WSLg** for graphical output
* A compatible game/ROM collection

### Without Docker (native build)

* A C++20 compiler (GCC or Clang)
* [CMake](https://cmake.org/) ≥ 3.20
* [Ninja](https://ninja-build.org/) (optional, but used by the Dockerfile)
* SDL2 and SDL2_ttf development packages

On Debian/Ubuntu:

```bash
sudo apt-get install build-essential cmake ninja-build libsdl2-dev libsdl2-ttf-dev
```

> **Note:** ROMs are not included in this repository. Only use ROMs that you legally own or have permission to use.

## 🔄 Continuous Integration

The project uses **GitHub Actions** to automatically verify changes.

The CI pipeline runs on every push to the `main` branch and on pull requests targeting `main`.

It performs the following checks:

* **Pre-commit**

  * C/C++ formatting with `clang-format`
  * Static analysis with `cppcheck`
  * General repository checks
* **Build**

  * Verifies that the project can be successfully built using the project's `Dockerfile`

The workflow is defined in:

```text
.github/workflows/checks.yml
```

A successful CI run ensures that the code passes the configured checks and that the Docker image can be built successfully.

## 🐳 Building

### With Docker

Build the Docker image from the project root:

```bash
docker build -t emulator-collection .
```

You can replace `emulator-collection` with any image name you prefer.

The image is based on Ubuntu and compiles the project with CMake and Ninja. The resulting binary is `build/emulators-collection`, which is the default command of the container.

### Natively

```bash
cmake -S . -B build -G Ninja
cmake --build build
./build/emulators-collection
```

> The build uses `-Werror`: any compiler warning fails the build.

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

The exact supported formats depend on the emulator being used. The game selector lists the ROMs found in `games/` and launches the matching emulator.

## ▶️ Running

### WSL2 + WSLg

If you are running the project through **WSL2**, the application can use WSLg to display its graphical interface.

Run:

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

The `games` directory is mounted into the container so that ROMs added on the host are available to the emulator.

### Native Linux

If you built the project natively, run it from the project root so that it can find the `games` and `assets` directories:

```bash
./build/emulators-collection
```

## 🏗️ Project Structure

```text
emulators-collection/
├── .github/
│   └── workflows/
│       └── checks.yml                 # GitHub Actions CI
├── assets/
│   └── fonts/                     # Fonts used by the UI (SDL2_ttf)
├── scripts/                       # Helper scripts
├── src/
│   ├── main.cc                    # Application entry point
│   ├── emulators/                 # Emulator implementations
│   │   ├── console/               # Full systems: chip_8, Atari2600, GameBoy
│   │   ├── CPU/                   # CPU cores: CPU65 (6502 family), LR35902 (Game Boy)
│   │   ├── BUS/                   # Memory buses: Atari2600Bus, GameBoyBus
│   │   ├── PPU/                   # Video chips: TIA1A (Atari), GameBoyPPU
│   │   ├── Processor/             # Support chips: MOS6532 (RIOT)
│   │   ├── Timer/                 # Timers: GameBoyTimer
│   │   ├── Controller/            # Input controllers: GameBoyController
│   │   └── Cartbridge/            # Cartridges and mappers: GameBoyCartbridge (MBC1/2/3/5)
│   ├── emulator_schema/           # Common emulator interface
│   ├── game/                      # Game/ROM representation
│   ├── game_selector/             # Game selection and management
│   └── UI/                        # Graphical user interface
├── games/                         # Local ROM collection (not versioned)
├── .clang-format                  # C/C++ formatting rules
├── .pre-commit-config.yaml        # Pre-commit configuration
├── .releaserc.json                # semantic-release configuration
├── .dockerignore
├── .gitignore
├── CMakeLists.txt                 # Build configuration
├── Dockerfile                     # Docker build configuration
├── Doxyfile                       # Doxygen configuration
├── LICENSE                        # MIT license
└── README.md
```

## 🧩 Architecture

The project separates the emulator implementations from the rest of the application.

```text
                 ┌─────────────────┐
                 │       UI        │
                 └────────┬────────┘
                          │
                 ┌────────▼────────┐
                 │ Game Selector   │
                 └────────┬────────┘
                          │
                 ┌────────▼────────┐
                 │    Emulator     │
                 │    Interface    │
                 └────────┬────────┘
                          │
            ┌─────────────┼─────────────┐
            │             │             │
       ┌────▼────┐   ┌────▼────┐   ┌────▼────┐
       │ CHIP-8  │   │ Atari   │   │  Game   │
       │         │   │  2600   │   │   Boy   │
       └─────────┘   └─────────┘   └─────────┘
```

This makes it possible to add new emulators without having to redesign the entire application.

### Inside a console emulator

Emulators that model real hardware are built from small, reusable components connected through a **bus**:

```text
                ┌──────────────┐
                │   Console    │  (e.g. GameBoy: main loop, SDL input/render)
                └──────┬───────┘
                       │
                ┌──────▼───────┐
                │     Bus      │  memory map + component wiring + timing
                └──────┬───────┘
     ┌─────────┬───────┼────────┬───────────┬──────────┐
┌────▼───┐ ┌───▼───┐ ┌─▼──┐ ┌───▼────┐ ┌────▼────┐ ┌───▼────┐
│  CPU   │ │  PPU  │ │RAM │ │ Timer  │ │Joypad   │ │Cartridge│
└────────┘ └───────┘ └────┘ └────────┘ └─────────┘ └────────┘
```

| Console      | CPU        | Video      | Other components                                   |
| ------------ | ---------- | ---------- | -------------------------------------------------- |
| **Atari 2600** | `CPU65`  | `TIA1A`    | `MOS6532` (RIOT: RAM, timer, I/O), `Atari2600Bus`  |
| **Game Boy** | `LR35902`  | `GameBoyPPU` | `GameBoyBus`, `GameBoyTimer`, `GameBoyController`, `GameBoyCartbridge` |

The Game Boy bus maps ROM, VRAM, external RAM, WRAM, OAM, I/O registers and HRAM, and handles OAM DMA and interrupts.

## ➕ Adding a New Emulator

1. Create the emulator class in `src/emulators/console/<name>/` by implementing the common interface found in `src/emulator_schema/`.
2. Add any reusable hardware components (CPU, bus, PPU, ...) in the matching `src/emulators/<category>/` folder.
3. Register the new `.cc` files and include directories in `CMakeLists.txt`.
4. Register the emulator in the game selector so that it is associated with its ROM file extensions.
5. Document new classes with Doxygen comments and add a row to the table at the top of this README.

## 📚 Documentation

The source code is documented with [Doxygen](https://www.doxygen.nl/). The configuration is in the `Doxyfile` at the project root.

Generate the documentation with:

```bash
sudo apt-get install doxygen graphviz   # graphviz is optional (diagrams)
doxygen Doxyfile
```

The generated HTML output can then be opened in your browser (see the `OUTPUT_DIRECTORY` setting in the `Doxyfile` for its location).

## 🛠️ Development

### Pre-commit

The project uses **pre-commit** to automatically check and format the code before commits.

#### Installation

Install `pre-commit` using `pip`:

```bash
pip install pre-commit
```

Then, from the project root, install the Git hooks:

```bash
pre-commit install
```

To manually run all checks on the entire repository:

```bash
pre-commit run --all-files
```

The pre-commit configuration is located in:

```text
.pre-commit-config.yaml
```

The formatting rules for C/C++ are defined in:

```text
.clang-format
```

### Code style and build flags

* C++20, no compiler extensions (`CMAKE_CXX_EXTENSIONS OFF`)
* Compiled with `-O3 -Wall -Wextra -Werror -pedantic -Wold-style-cast`: use `static_cast<>` instead of C-style casts
* Format your code with `clang-format` (done automatically by pre-commit)

### Releases

Releases are automated with [semantic-release](https://semantic-release.gitbook.io/) (configured in `.releaserc.json`) on the `main` branch: commit messages are analysed to generate release notes and GitHub releases. Using [Conventional Commits](https://www.conventionalcommits.org/) is recommended:

```text
feat(gameboy): add MBC3 real-time clock
fix(atari2600): correct TIA sprite positioning
docs: update README
```

### Roadmap

The project is primarily developed in **C++** and uses Docker to provide a reproducible build environment.

Current development focuses on:

* Improving emulator accuracy
* Implementing additional CPU instructions
* Implementing graphics and input systems
* Completing the Game Boy emulator (PPU accuracy, audio, more mappers)
* Expanding ROM/game support
* Improving the emulator abstraction
* Adding additional console and computer emulators

## 🤝 Contributing

Contributions are welcome.

1. Fork the repository and create a feature branch.
2. Make your changes, following the code style described above.
3. Run `pre-commit run --all-files` and make sure the Docker build passes.
4. Open a pull request targeting `main`.

## 📄 License

This project is licensed under the **MIT License**.

See the [LICENSE](LICENSE) file for more information.
