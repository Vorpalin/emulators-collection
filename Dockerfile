# Build the WebAssembly modules with the Emscripten toolchain.
FROM emscripten/emsdk:6.0.13 AS wasm-builder

WORKDIR /src

# Avoid apt-get upgrade: it invalidates caches and makes builds less reproducible.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        cmake \
        ninja-build \
        git \
    && rm -rf /var/lib/apt/lists/*

# Copy only files required by CMake to keep the WASM build cache reusable.
COPY CMakeLists.txt ./
COPY src/emulators ./src/emulators
COPY src/wasm ./src/wasm

RUN emcmake cmake \
        -S . \
        -B build-wasm \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build-wasm --parallel \
    && mkdir -p /out/wasm \
    && cp build-wasm/emulators.js build-wasm/emulators.wasm /out/wasm/

# The development image only needs Node/Vite; do not ship the large Emscripten SDK.
FROM node:24-bookworm-slim AS dev

WORKDIR /src

ENV NODE_ENV=development

# Cache npm dependencies independently from source changes.
COPY web/package.json web/package-lock.json ./web/
RUN --mount=type=cache,target=/root/.npm \
    npm --prefix web ci --prefer-offline

COPY web ./web
COPY --from=wasm-builder /out/wasm ./web/src/wasm

EXPOSE 5173

CMD ["npm", "--prefix", "web", "run", "dev", "--", "--host", "0.0.0.0"]
