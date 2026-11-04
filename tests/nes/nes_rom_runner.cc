// Headless runner for NES test ROMs that follow blargg's reporting protocol:
//   $6001-$6003 = DE B0 61 once the protocol is active,
//   $6000       = 0x80 while running, then the result code (0 = pass),
//   $6004...    = NUL-terminated text.
//
// Usage: nes_rom_runner <rom.nes> [max_frames] [dump.ppm]
// Exit code: 0 = pass, 1 = fail, 2 = no result / error.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "emulators/bus/nes_bus/NesBus.hh"

static void DumpPpm(const NesBus& bus, const char* path) {
  static const uint32_t kPal[64] = {
      0x666666, 0x002A88, 0x1412A7, 0x3B00A4, 0x5C007E, 0x6E0040, 0x6C0600,
      0x561D00, 0x333500, 0x0B4800, 0x005200, 0x004F08, 0x00404D, 0x000000,
      0x000000, 0x000000, 0xADADAD, 0x155FD9, 0x4240FF, 0x7527FE, 0xA01ACC,
      0xB71E7B, 0xB53120, 0x994E00, 0x6B6D00, 0x388700, 0x0C9300, 0x008F32,
      0x007C8D, 0x000000, 0x000000, 0x000000, 0xFFFEFF, 0x64B0FF, 0x9290FF,
      0xC676FF, 0xF36AFF, 0xFE6ECC, 0xFE8170, 0xEA9E22, 0xBCBE00, 0x88D800,
      0x5CE430, 0x45E082, 0x48CDDE, 0x4F4F4F, 0x000000, 0x000000, 0xFFFEFF,
      0xC0DFFF, 0xD3D2FF, 0xE8C8FF, 0xFBC2FF, 0xFEC4EA, 0xFECCC5, 0xF7D8A5,
      0xE4E594, 0xCFEF96, 0xBDF4AB, 0xB3F3CC, 0xB5EBF2, 0xB8B8B8, 0x000000,
      0x000000};
  std::ofstream out(path, std::ios::binary);
  out << "P6\n256 240\n255\n";
  for (uint16_t px : bus.ppu().frameBuffer()) {
    const uint32_t c = kPal[px & 0x3F];
    const char rgb[3] = {static_cast<char>(c >> 16), static_cast<char>(c >> 8),
                         static_cast<char>(c)};
    out.write(rgb, 3);
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s rom.nes [max_frames] [dump.ppm]\n",
                 argv[0]);
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  const std::vector<uint8_t> rom((std::istreambuf_iterator<char>(in)), {});
  const int max_frames = argc > 2 ? std::atoi(argv[2]) : 3000;

  NesBus bus;
  if (!bus.loadROM(rom.data(), rom.size())) {
    std::fprintf(stderr, "load failed: %s\n", bus.error().c_str());
    return 2;
  }

  int status = -1;
  for (int frame = 0; frame < max_frames; ++frame) {
    do {
      bus.step();
    } while (!bus.consumeFrameReady());

    const bool active = bus.cartridge().cpuRead(0x6001) == 0xDE &&
                        bus.cartridge().cpuRead(0x6002) == 0xB0 &&
                        bus.cartridge().cpuRead(0x6003) == 0x61;
    if (!active) continue;
    const int s = bus.cartridge().cpuRead(0x6000);
    // 0x81 = "reset the console and call again": not needed for these ROMs.
    if (s != 0x80 && s != 0x81) {
      status = s;
      break;
    }
  }

  if (argc > 3) DumpPpm(bus, argv[3]);

  std::string text;
  for (uint16_t a = 0x6004; a < 0x7FFF; ++a) {
    const char ch = static_cast<char>(bus.cartridge().cpuRead(a));
    if (ch == 0) break;
    text += ch;
  }
  std::printf("%s\n[status=%d]\n", text.c_str(), status);
  return status == 0 ? 0 : (status < 0 ? 2 : 1);
}
