#!/usr/bin/env bash
# Compile les cœurs C++ en WebAssembly et copie le résultat dans web/public/wasm.
# À lancer depuis la racine du dépôt (là où se trouve le CMakeLists.txt) :
#   ./web/scripts/build-wasm.sh
set -euo pipefail

emcmake cmake -S . -B build-wasm -DCMAKE_BUILD_TYPE=Release
cmake --build build-wasm -j

mkdir -p web/public/wasm
cp build-wasm/emulators.js build-wasm/emulators.wasm web/public/wasm/
echo "OK -> web/public/wasm/emulators.{js,wasm}"
