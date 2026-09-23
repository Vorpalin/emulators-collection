# Emulators Collection

A collection of classic video game console and computer emulators written in **C++**.

The project is designed with a modular architecture so that different emulators can share common components such as game management, input handling, and the user interface.

## 🎮 Available Emulators

| Emulator           | Status         | Description                                                                 |
| ------------------ | -------------- | --------------------------------------------------------------------------- |
| **CHIP-8**         | ✅ Available    | CHIP-8 interpreter                                                          |
| **Super-CHIP 48**  | ✅ Available | Extended CHIP-8 interpreter with 128×64 display and additional instructions |
| **Atari 2600**     | ✅ Available | Atari 2600 emulator                                                         |

More emulators will be added over time.

## ✨ Features

* Modular emulator architecture
* C++ implementation
* Docker-based development and execution
* Game/ROM selection system
* SDL-based graphical interface
* Support for multiple emulators within the same application
* Designed to be easily extended with new systems

## 📋 Requirements

* [Docker](https://www.docker.com/)
* Linux or **WSL2 with WSLg** for graphical output
* A compatible game/ROM collection

> **Note:** ROMs are not included in this repository. Only use ROMs that you legally own or have permission to use.

## 🐳 Building

Build the Docker image from the project root:

```bash
docker build -t emulator-collection .
```

You can replace `emulator-collection` with any image name you prefer.

## 🎮 Adding Games

Create the `games` directory if it does not already exist:

```bash
mkdir -p games
```

Then place your compatible ROMs inside it:

```text
games/
├── game1.ch8
├── game2.ch8
└── ...
```

The exact supported formats depend on the emulator being used.

## ▶️ Running

### WSL2 + WSLg

If you are running the project through **WSL2**, the application can use WSLg to display its graphical interface.

Run:

```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
  -e XDG_RUNTIME_DIR=/tmp/runtime \
  -v /mnt/wslg/runtime-dir:/tmp/runtime \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v "$(pwd)/games:/app/games" \
  emulator-collection
```

The `games` directory is mounted into the container so that ROMs added on the host are available to the emulator.

## 🏗️ Project Structure

```text
emulators-collection/
├── src/
│   ├── main.cc                 # Application entry point
│   ├── emulators/              # Emulator implementations
│   │   ├── Chip8/
│   │   └── ...
│   ├── emulator_schema/        # Common emulator interface
│   ├── game/                   # Game/ROM representation
│   ├── game_selector/          # Game selection and management
│   └── UI/                     # Graphical user interface
│
├── games/                      # Local ROM collection
├── Dockerfile                  # Docker build configuration
├── CMakeLists.txt              # Build configuration
└── LICENSE                     # MIT license
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
       │ CHIP-8  │   │ Atari   │   │  ...    │
       │         │   │  2600   │   │         │
       └─────────┘   └─────────┘   └─────────┘
```

This makes it possible to add new emulators without having to redesign the entire application.

## 🛠️ Development

The project is primarily developed in **C++** and uses Docker to provide a reproducible build environment.

Current development focuses on:

* Improving emulator accuracy
* Implementing additional CPU instructions
* Implementing graphics and input systems
* Expanding ROM/game support
* Improving the emulator abstraction
* Adding additional console and computer emulators

## 📄 License

This project is licensed under the **MIT License**.

See the [LICENSE](LICENSE) file for more information.
