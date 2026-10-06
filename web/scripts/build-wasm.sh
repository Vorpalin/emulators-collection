#!/usr/bin/env bash

set -euo pipefail

emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j

mkdir -p web/public/wasm
cp build-wasm/emulators.js build-wasm/emulators.wasm web/public/wasm/
echo "OK -> web/public/wasm/emulators.{js,wasm}"
