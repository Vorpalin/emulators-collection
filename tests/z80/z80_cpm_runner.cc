// Runs a CP/M .com program (prelim, zexdoc, zexall) on the Z80 core.
//
// Only the two BDOS calls those programs use are emulated:
//   C = 2: print the character in E
//   C = 9: print the '$'-terminated string at DE
// The program exits when it jumps to address 0 (warm boot). The process
// returns 1 if the output contains "ERROR" or "failed", 0 otherwise.
//
// Usage: z80_cpm_runner <program.com>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "emulators/cpu/Z80/Z80.hh"

namespace {

class FlatBus : public Z80Bus {
 public:
  FlatBus() : mem(0x10000, 0) {}
  void loadROM(std::string &) override {}
  void reset() override {}
  void tick() override {}
  uint8_t read(uint16_t a) override { return mem[a]; }
  void write(uint16_t a, uint8_t v) override { mem[a] = v; }
  uint8_t readPort(uint16_t) override { return 0xFF; }
  void writePort(uint16_t, uint8_t) override {}
  std::vector<uint8_t> mem;
};

}  // namespace

int main(int argc, char **argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: %s <program.com>\n", argv[0]);
    return 2;
  }
  std::ifstream in(argv[1], std::ios::binary);
  if (!in) {
    std::fprintf(stderr, "cannot open %s\n", argv[1]);
    return 2;
  }
  std::vector<uint8_t> prog((std::istreambuf_iterator<char>(in)),
                            std::istreambuf_iterator<char>());

  FlatBus bus;
  for (size_t i = 0; i < prog.size() && 0x100 + i < 0x10000; i++)
    bus.mem[0x100 + i] = prog[i];
  bus.mem[0x0005] = 0xC9;  // RET: BDOS entry, intercepted below.

  Z80 cpu(&bus);
  Z80State st = cpu.getState();
  st.PC = 0x100;
  st.SP = 0xFFFE;
  cpu.setState(st);

  std::string output;
  uint64_t tstates = 0;
  for (;;) {
    uint16_t pc = cpu.getPC();
    if (pc == 0x0000) break;
    if (pc == 0x0005) {
      Z80State s = cpu.getState();
      uint8_t fn = s.reg[1];  // C
      if (fn == 2) {
        char ch = static_cast<char>(s.reg[3]);  // E
        output.push_back(ch);
        std::putchar(ch);
      } else if (fn == 9) {
        uint16_t addr = static_cast<uint16_t>((s.reg[2] << 8) | s.reg[3]);
        while (bus.mem[addr] != '$') {
          output.push_back(static_cast<char>(bus.mem[addr]));
          std::putchar(static_cast<char>(bus.mem[addr]));
          addr++;
        }
      }
      std::fflush(stdout);
    }
    tstates += cpu.execute();
  }
  std::printf("\n[%llu T-states]\n", static_cast<unsigned long long>(tstates));

  bool failed = output.find("ERROR") != std::string::npos ||
                output.find("failed") != std::string::npos ||
                output.find("FAILED") != std::string::npos;
  return failed ? 1 : 0;
}
