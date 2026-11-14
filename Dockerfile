# Build the WebAssembly modules with the Emscripten toolchain.
FROM emscripten/emsdk:6.0.13 AS wasm-builder

ARG WARNINGS_AS_ERRORS=OFF

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
        -DWARNINGS_AS_ERRORS=${WARNINGS_AS_ERRORS} \
    && cmake --build build-wasm --parallel \
    && mkdir -p /out/wasm \
    && cp build-wasm/emulators.js build-wasm/emulators.wasm /out/wasm/

# Node, the npm dependencies, the web sources and the WebAssembly output: everything the
# development server and the production build have in common. Keep this on the same Node
# major as CI (.github/workflows/checks.yml) and the README.
FROM node:24-bookworm-slim AS web-base

WORKDIR /src

# Cache npm dependencies independently from source changes.
COPY web/package.json web/package-lock.json ./web/
RUN --mount=type=cache,target=/root/.npm \
    npm --prefix web ci --prefer-offline

COPY web ./web
COPY --from=wasm-builder /out/wasm ./web/src/wasm

# Build the static site. Vite inlines the VITE_* values into the bundle, so they must be
# given at build time: --build-arg VITE_SUPABASE_URL=... --build-arg VITE_SUPABASE_ANON_KEY=...
# (the anon key is a public client key, it ends up in the shipped JavaScript anyway).
FROM web-base AS site-builder

ARG VITE_BASE=/
ARG VITE_SUPABASE_URL
ARG VITE_SUPABASE_ANON_KEY

RUN if [ -z "${VITE_SUPABASE_URL}" ] || [ -z "${VITE_SUPABASE_ANON_KEY}" ]; then \
        echo "error: pass --build-arg VITE_SUPABASE_URL and --build-arg VITE_SUPABASE_ANON_KEY" >&2; \
        exit 1; \
    fi

RUN VITE_BASE="${VITE_BASE}" \
    VITE_SUPABASE_URL="${VITE_SUPABASE_URL}" \
    VITE_SUPABASE_ANON_KEY="${VITE_SUPABASE_ANON_KEY}" \
    npm --prefix web run build

# Production image: only the static files and an unprivileged web server. Neither Node,
# the npm packages nor the Emscripten SDK end up in it.
#   docker build --target production -t emulators-collection:prod \
#       --build-arg VITE_SUPABASE_URL=... --build-arg VITE_SUPABASE_ANON_KEY=... .
FROM nginxinc/nginx-unprivileged:1.28-alpine AS production

COPY docker/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=site-builder /src/web/dist /usr/share/nginx/html

EXPOSE 8080

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
    CMD wget -q --spider http://127.0.0.1:8080/ || exit 1

# Development image (keep it the LAST stage: it is what a plain `docker build` produces,
# which is what the CI build job relies on). It only needs Node/Vite; the large Emscripten
# SDK and the native build tools are not included.
FROM web-base AS dev

ENV NODE_ENV=development

EXPOSE 5173

CMD ["npm", "--prefix", "web", "run", "dev", "--", "--host", "0.0.0.0"]
