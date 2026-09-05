## Emulator collections

A collection of emulator written in C++

## Requirement

- Docker

## Building

```bash
docker image build -t <image-name> .
```

## Running

```bash
docker run --rm -it \
  -e DISPLAY=$DISPLAY \
  -e WAYLAND_DISPLAY=$WAYLAND_DISPLAY \
  -e XDG_RUNTIME_DIR=/tmp/runtime \
  -v /mnt/wslg/runtime-dir:/tmp/runtime \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  <image-name> \
```

## Project Structure

```
emulators-collection
├── src/
│   ├── main.cc             # Entry point
│   ├── CHIP-8              # CHIP 8 emulator
│   └── emulator_schema     # Emulator Schema
```

## Licence

[MIT Licence](LICENSE)
