#!/usr/bin/env sh
# Downloads the public test ROMs used by the CTest suite, pinned to exact
# commits so results are reproducible. ROMs are never committed to this repo.
#
# Usage: sh tests/fetch_roms.sh [destination]   (default: build-tests/test-roms)
set -eu

DEST="${1:-build-tests/test-roms}"

fetch() { # name url sha
  dir="$DEST/$1"
  if [ -d "$dir/.git" ] && [ "$(git -C "$dir" rev-parse HEAD 2>/dev/null)" = "$3" ]; then
    echo "$1: already at $3"
    return
  fi
  rm -rf "$dir"
  mkdir -p "$dir"
  git -C "$dir" init -q
  git -C "$dir" remote add origin "$2"
  git -C "$dir" fetch -q --depth 1 origin "$3"
  git -C "$dir" checkout -q FETCH_HEAD
  echo "$1: fetched $3"
}

# Single release asset (the ROM is not committed upstream), pinned by SHA-256.
fetch_file() { # name url sha256
  dir="$DEST/$1"
  file="$dir/$(basename "$2")"
  if [ -f "$file" ] && [ "$(sha256sum "$file" | cut -d' ' -f1)" = "$3" ]; then
    echo "$1: already present"
    return
  fi
  mkdir -p "$dir"
  curl -fsSL -o "$file" "$2"
  echo "$3  $file" | sha256sum -c - >/dev/null || { rm -f "$file"; echo "$1: checksum mismatch" >&2; exit 1; }
  echo "$1: fetched $(basename "$2")"
}

fetch nes   https://github.com/christopherpow/nes-test-roms.git 95d8f621ae55cee0d09b91519a8989ae0e64753b
fetch gb    https://github.com/retrio/gb-test-roms.git           c240dd7d700e5c0b00a7bbba52b53e4ee67b5f15
fetch chip8 https://github.com/Timendus/chip8-test-suite.git     742e9eac9f5d44466c1c9d83fcaa8158c6683cb1
fetch z80   https://github.com/superzazu/z80.git                d64fe10a2274e5e40019b1086bf7d8990cbc5f23
fetch_file gb-acid2 https://github.com/mattcurrie/cgb-acid2/releases/download/v1.1/cgb-acid2.gbc 197fb0bcec544f0400527fc707e0a94f55435974986e6986b424ace5de81720e
