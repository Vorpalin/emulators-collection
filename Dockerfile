FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libsdl2-dev \
    && rm -rf /var/lib/apt/lists/*

WORkDIR /app

COPY . .

RUN mkdir build && cd build \
    && cmake -G "Ninja" .. \
    && cmake --build .

CMD ["./build/emulators-collection"]