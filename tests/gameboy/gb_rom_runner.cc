// Headless runner for Game Boy test ROMs that report through either
//   - the serial port (Blargg cpu_instrs / instr_timing / interrupt_time...):
//     the ROM prints "Passed" or "Failed" followed by a newline, or
//   - the cartridge RAM protocol (Blargg mem_timing-2, dmg_sound...):
//     $A001-$A003 = DE B0 61 once active, $A000 = 0x80 while running then the
//     result code (0 = pass), $A004... = NUL-terminated text.
//
// Usage: gb_rom_runner <rom.gb> [max_frames]
// Exit code: 0 = pass, 1 = fail, 2 = no result / error.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "emulators/console/game_boy/GameBoy.hh"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s rom.gb [max_frames]\n", argv[0]);
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  const std::vector<uint8_t> rom((std::istreambuf_iterator<char>(in)), {});
  const int max_frames = argc > 2 ? std::atoi(argv[2]) : 6000;

  GameBoy gb;
  if (rom.empty() || !gb.loadProgram(rom.data(), rom.size())) {
    std::fprintf(stderr, "load failed: %s\n", argv[1]);
    return 2;
  }
  GameBoyBus& bus = gb.getBus();

  int status = -1;
  std::string text;
  for (int frame = 0; frame < max_frames && status < 0; ++frame) {
    gb.stepFrame();

    // Cartridge-RAM protocol.
    if (bus.read(0xA001) == 0xDE && bus.read(0xA002) == 0xB0 &&
        bus.read(0xA003) == 0x61) {
      const int s = bus.read(0xA000);
      if (s != 0x80 && s != 0x81) {
        status = s;
        text.clear();
        for (uint16_t a = 0xA004; a < 0xBFFF; ++a) {
          const char ch = static_cast<char>(bus.read(a));
          if (ch == 0) break;
          text += ch;
        }
        break;
      }
    }

    // Serial protocol.
    const std::string& serial = bus.serialOutput();
    if (serial.find("Passed") != std::string::npos) {
      status = 0;
      text = serial;
    } else if (serial.find("Failed") != std::string::npos) {
      status = 1;
      text = serial;
    }
  }

  if (status < 0) text = bus.serialOutput();
  std::printf("%s\n[status=%d]\n", text.c_str(), status);
  return status == 0 ? 0 : (status < 0 ? 2 : 1);
}
