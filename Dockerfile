FROM emscripten/emsdk:6.0.10

WORKDIR /src

# Tools needed by the project
RUN apt-get update \
    && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends \
        cmake \
        ninja-build \
        git \
    && rm -rf /var/lib/apt/lists/*

# Copy the project
COPY . .

# Install frontend dependencies
WORKDIR /src/web

RUN npm ci

WORKDIR /src

# Build the WebAssembly version
RUN emcmake cmake \
        -S . \
        -B build-wasm \
        -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build-wasm -j"$(nproc)" \
    && mkdir -p web/src/wasm \
    && cp build-wasm/emulators.js \
       build-wasm/emulators.wasm \
       web/src/wasm/

# Vite
EXPOSE 5173

CMD ["npm", "--prefix", "web", "run", "dev", "--", "--host", "0.0.0.0"]
