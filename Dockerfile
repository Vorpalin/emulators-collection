ARG UBUNTU_VERSION=24.04

# ---- Toolchain ------------------------------------------------------------
FROM ubuntu:${UBUNTU_VERSION} AS toolchain

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get update -y && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        ninja-build \
        libsdl2-dev \
        libsdl2-ttf-dev \
        libgtest-dev \
    && rm -rf /var/lib/apt/lists/*

# ---- Build -----------------------------------------------------------------
FROM toolchain AS build

ARG BUILD_TYPE=Release

WORKDIR /app

COPY CMakeLists.txt ./
COPY src ./src

RUN cmake -S . -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

RUN cmake --build build

# ---- Runtime (no compiler, no headers, no test binaries) -------------------
FROM ubuntu:${UBUNTU_VERSION} AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get update -y && apt-get install -y --no-install-recommends \
        libsdl2-2.0-0 \
        libsdl2-ttf-2.0-0 \
        libpulse0 \
        libgl1 \
        libegl1 \
        libgles2 \
        libglx-mesa0 \
        libegl-mesa0 \
        libgl1-mesa-dri \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY assets ./assets
COPY --from=build /app/build/emulators-collection ./emulators-collection

CMD ["./emulators-collection"]
