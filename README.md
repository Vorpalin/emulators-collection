## Emulator collections

A collection of emulator written in C++

## Available Emulators

- CHIP-8/Super Chip-48

## Requirement

- Docker

## Building

```bash
docker image build -t <image-name> .
```

## Configuration

1. Create the games directory

```bash
mkdir games
```

2. Add all the compatible games your want

## Running

```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
  -e XDG_RUNTIME_DIR=/tmp/runtime \
  -v /mnt/wslg/runtime-dir:/tmp/runtime \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  <image-name>
```

## Project Structure

```
emulators-collection
├── src/
│   ├── main.cc             # Entry point
│   ├── emulators           # Available emulators
│   ├── emulator_schema     # Emulator Schema
│   ├── game                # Game representation
│   ├── game_selector       # Game handler
│   └── UI                  # UI of the app
└── games/                  # Your collection of games
```

## Licence

[MIT Licence](LICENSE)
