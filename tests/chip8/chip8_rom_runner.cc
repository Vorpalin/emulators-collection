// Headless runner for CHIP-8 test ROMs that report on screen only
// (Timendus' chip8-test-suite). Runs a fixed number of 60 Hz frames, then
// compares an FNV-1a hash of the framebuffer with the expected one.
//
// Usage: chip8_rom_runner <rom.ch8> <frames> [expected_hash_hex] [--ascii]
// Exit code: 0 = hash matches (or none given), 1 = mismatch, 2 = error.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <vector>

#include "emulators/console/chip8/Chip8.hh"

static uint64_t Fnv1a(const uint8_t* data, std::size_t n) {
  uint64_t h = 0xcbf29ce484222325ULL;
  for (std::size_t i = 0; i < n; ++i) {
    h ^= data[i];
    h *= 0x100000001b3ULL;
  }
  return h;
}

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr,
                 "usage: %s rom.ch8 frames [expected_hash_hex] [--ascii]\n",
                 argv[0]);
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  const std::vector<uint8_t> rom((std::istreambuf_iterator<char>(in)), {});
  const int frames = std::atoi(argv[2]);

  bool ascii = false;
  const char* expected = nullptr;
  for (int i = 3; i < argc; ++i) {
    if (std::strcmp(argv[i], "--ascii") == 0) {
      ascii = true;
    } else {
      expected = argv[i];
    }
  }

  Chip8 chip8;
  if (rom.empty() || !chip8.loadProgram(rom.data(), rom.size())) {
    std::fprintf(stderr, "load failed: %s\n", argv[1]);
    return 2;
  }
  for (int f = 0; f < frames; ++f) chip8.stepFrame();

  const int w = chip8.width();
  const int h = chip8.height();
  const uint8_t* fb = chip8.framebuffer();
  const uint64_t hash = Fnv1a(fb, static_cast<std::size_t>(w) * h * 4);

  if (ascii) {
    for (int y = 0; y < h; ++y) {
      for (int x = 0; x < w; ++x) {
        const uint8_t* p = fb + (static_cast<std::size_t>(y) * w + x) * 4;
        std::putchar((p[0] | p[1] | p[2]) > 0x40 ? '#' : '.');
      }
      std::putchar('\n');
    }
  }
  std::printf("hash=%016" PRIx64 "\n", hash);

  if (!expected) return 0;
  const uint64_t want = std::strtoull(expected, nullptr, 16);
  return want == hash ? 0 : 1;
}
