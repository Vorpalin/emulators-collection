// Headless runner for Game Boy ROMs that report on screen only (e.g.
// cgb-acid2): runs a number of frames and compares an FNV-1a hash of the final
// RGBA framebuffer with a golden value. Works in DMG and CGB mode.
//
// Usage: gb_frame_runner <rom> <frames> [expected_hash_hex]
// Exit code: 0 = hash matches (or none given), 1 = mismatch, 2 = error.
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

#include "emulators/console/game_boy/GameBoy.hh"

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
    std::fprintf(stderr, "usage: %s rom frames [expected_hash_hex]\n", argv[0]);
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  const std::vector<uint8_t> rom((std::istreambuf_iterator<char>(in)), {});
  GameBoy gb;
  if (rom.empty() || !gb.loadProgram(rom.data(), rom.size())) {
    std::fprintf(stderr, "load failed: %s\n", argv[1]);
    return 2;
  }
  const int frames = std::atoi(argv[2]);
  for (int i = 0; i < frames; ++i) gb.stepFrame();

  const uint64_t hash = Fnv1a(gb.framebuffer(), 160 * 144 * 4);
  std::printf("cgb=%d hash=%016" PRIx64 "\n", gb.getBus().cgbMode() ? 1 : 0,
              hash);
  if (argc < 4) return 0;
  const uint64_t want = std::strtoull(argv[3], nullptr, 16);
  return want == hash ? 0 : 1;
}
