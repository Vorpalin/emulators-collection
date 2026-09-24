FROM ubuntu:25.10

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libsdl2-dev \
    libsdl2-ttf-dev \
    libpulse0 \
    pulseaudio \
    pulseaudio-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S . -B build -G Ninja

RUN cmake --build build

CMD ["./build/emulators-collection"]
