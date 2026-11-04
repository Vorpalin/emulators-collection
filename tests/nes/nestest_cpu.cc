// CPU-only check of the 6502 core against the reference trace of nestest.
//
// Runs nestest.nes in "automation" mode (PC=$C000) and compares PC, A, X, Y,
// P, SP and the CPU cycle counter with nestest.log after every instruction.
//
// Usage: nestest_cpu <nestest.nes> <nestest.log>
// Exit code: 0 = identical, 1 = mismatch, 2 = usage / file error.
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "emulators/bus/Bus.hh"
#include "emulators/cpu/CPU65/CPU65.hh"

namespace {

/// 2 KiB RAM mirrored over $0000-$1FFF and the PRG ROM mirrored at $8000+.
class FlatBus : public Bus {
 public:
  std::vector<uint8_t> prg;
  uint8_t ram[0x800] = {};

  void loadROM(std::string&) override {}
  void reset() override {}
  void tick() override {}
  uint8_t read(uint16_t a) override {
    if (a < 0x2000) return ram[a & 0x7FF];
    if (a >= 0x8000 && !prg.empty()) return prg[(a - 0x8000) % prg.size()];
    return 0;
  }
  void write(uint16_t a, uint8_t v) override {
    if (a < 0x2000) ram[a & 0x7FF] = v;
  }
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: %s nestest.nes nestest.log\n", argv[0]);
    return 2;
  }
  std::ifstream rom_file(argv[1], std::ios::binary);
  const std::vector<uint8_t> rom((std::istreambuf_iterator<char>(rom_file)),
                                 std::istreambuf_iterator<char>());
  std::ifstream log(argv[2]);
  if (rom.size() < 16 + 0x4000 || !log) {
    std::fprintf(stderr, "cannot read input files\n");
    return 2;
  }

  FlatBus bus;
  bus.prg.assign(rom.begin() + 16, rom.begin() + 16 + 0x4000);
  CPU65 cpu(&bus);
  cpu.setDecimalModeEnabled(false);

  CPU65State s = cpu.getState();
  s.PC = 0xC000;
  s.SP = 0xFD;
  s.A = s.X = s.Y = 0;
  s.C = s.Z = s.D = s.B = s.V = s.N = false;
  s.I = true;
  cpu.setState(s);

  std::string line;
  uint64_t cycles = 7;  // the reset sequence
  int n = 0;
  while (std::getline(log, line)) {
    unsigned pc = 0, a = 0, x = 0, y = 0, p = 0, sp = 0;
    unsigned long long cyc = 0;
    std::sscanf(line.c_str(), "%x", &pc);
    const size_t regs = line.find("A:");
    const size_t cy = line.find("CYC:");
    if (regs == std::string::npos || cy == std::string::npos) continue;
    std::sscanf(line.c_str() + regs, "A:%x X:%x Y:%x P:%x SP:%x", &a, &x, &y,
                &p, &sp);
    std::sscanf(line.c_str() + cy, "CYC:%llu", &cyc);

    s = cpu.getState();
    const unsigned flags =
        (s.N << 7) | (s.V << 6) | (s.D << 3) | (s.I << 2) | (s.Z << 1) | s.C;
    // Bits 4 and 5 do not exist in the register; ignore them.
    const bool ok = s.PC == pc && s.A == a && s.X == x && s.Y == y &&
                    flags == (p & 0xCF) && s.SP == sp && cycles == cyc;
    if (!ok) {
      std::printf(
          "MISMATCH at line %d\n  expected: %s\n  got:      PC:%04X A:%02X "
          "X:%02X Y:%02X P:%02X SP:%02X CYC:%llu\n",
          n + 1, line.c_str(), s.PC, s.A, s.X, s.Y, flags, s.SP,
          static_cast<unsigned long long>(cycles));
      return 1;
    }
    cycles += cpu.execute();
    ++n;
  }
  std::printf(
      "PASS: %d instructions match nestest.log (registers, flags, cycles)\n",
      n);
  return 0;
}
