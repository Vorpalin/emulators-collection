// Z80 core checks on tiny hand-assembled programs: results, flags, T-state
// timings, interrupts and undocumented behaviour. No downloaded ROM needed.

#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <vector>

#include "emulators/cpu/Z80/Z80.hh"

namespace {

class TestBus : public Z80Bus {
 public:
  TestBus() : mem(0x10000, 0) {}
  void loadROM(std::string &) override {}
  void reset() override {}
  void tick() override {}
  uint8_t read(uint16_t a) override { return mem[a]; }
  void write(uint16_t a, uint8_t v) override { mem[a] = v; }
  uint8_t readPort(uint16_t p) override {
    lastPortRead = p;
    return 0x5A;
  }
  void writePort(uint16_t p, uint8_t v) override {
    lastPortWrite = p;
    lastPortValue = v;
  }
  std::vector<uint8_t> mem;
  uint16_t lastPortRead = 0, lastPortWrite = 0;
  uint8_t lastPortValue = 0;
};

int failures = 0;

#define CHECK(cond)                                                       \
  do {                                                                    \
    if (!(cond)) {                                                        \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);         \
      failures++;                                                         \
    }                                                                     \
  } while (0)

constexpr size_t B = 0, C = 1, D = 2, E = 3, H = 4, L = 5, F = 6, A = 7;

struct Rig {
  TestBus bus;
  Z80 cpu{&bus};

  explicit Rig(std::initializer_list<uint8_t> code, uint16_t at = 0x0100) {
    uint16_t a = at;
    for (uint8_t b : code) bus.mem[a++] = b;
    Z80State s = cpu.getState();
    s.PC = at;
    s.SP = 0xF000;
    s.reg.fill(0);
    s.alt.fill(0);
    cpu.setState(s);
  }
  Z80State st() const { return cpu.getState(); }
  void set(const Z80State &s) { cpu.setState(s); }
  uint32_t step() { return cpu.execute(); }
};

void testTimings() {
  CHECK(Rig({0x00}).step() == 4);                    // NOP
  CHECK(Rig({0x3E, 0x12}).step() == 7);              // LD A,n
  CHECK(Rig({0x36, 0x12}).step() == 10);             // LD (HL),n
  CHECK(Rig({0xDD, 0x7E, 0x05}).step() == 19);       // LD A,(IX+d)
  CHECK(Rig({0xDD, 0x34, 0x05}).step() == 23);       // INC (IX+d)
  CHECK(Rig({0xDD, 0x36, 0x05, 0x99}).step() == 19); // LD (IX+d),n
  CHECK(Rig({0x09}).step() == 11);                   // ADD HL,BC
  CHECK(Rig({0xC5}).step() == 11);                   // PUSH BC
  CHECK(Rig({0xC1}).step() == 10);                   // POP BC
  CHECK(Rig({0xCD, 0x00, 0x02}).step() == 17);       // CALL nn
  CHECK(Rig({0xE3}).step() == 19);                   // EX (SP),HL
  CHECK(Rig({0xDB, 0x10}).step() == 11);             // IN A,(n)
  CHECK(Rig({0xD3, 0x10}).step() == 11);             // OUT (n),A
  CHECK(Rig({0xED, 0x4A}).step() == 15);             // ADC HL,BC
  CHECK(Rig({0xED, 0x44}).step() == 8);              // NEG
  CHECK(Rig({0xED, 0x00}).step() == 8);              // undefined ED = NOP
  CHECK(Rig({0xCB, 0x46}).step() == 12);             // BIT 0,(HL)
  CHECK(Rig({0xCB, 0xC6}).step() == 15);             // SET 0,(HL)
  CHECK(Rig({0xDD, 0xCB, 0x01, 0x46}).step() == 20); // BIT 0,(IX+d)
  CHECK(Rig({0xDD, 0xCB, 0x01, 0xC6}).step() == 23); // SET 0,(IX+d)
  CHECK(Rig({0xC7}).step() == 11);                   // RST 00

  Rig djnz({0x10, 0x05});  // DJNZ: B=0 -> 255, taken
  CHECK(djnz.step() == 13);
  Rig djnz2({0x10, 0x05});
  Z80State s = djnz2.st();
  s.reg[B] = 1;
  djnz2.set(s);
  CHECK(djnz2.step() == 8);  // falls through

  Rig retnz({0xC0});  // RET NZ with Z set: not taken, 5 T-states
  s = retnz.st();
  s.reg[F] = 0x40;
  retnz.set(s);
  CHECK(retnz.step() == 5);
  Rig retnz2({0xC0});  // Z clear: taken, 11 T-states
  retnz2.bus.mem[0xF000] = 0x00;
  retnz2.bus.mem[0xF001] = 0x02;
  CHECK(retnz2.step() == 11);
  CHECK(retnz2.cpu.getPC() == 0x0200);

  Rig jr({0x20, 0x05});  // JR NZ taken (Z clear)
  s = jr.st();
  s.reg[F] = 0;
  jr.set(s);
  CHECK(jr.step() == 12);

  Rig ldir({0xED, 0xB0});  // LDIR with BC = 2: repeats once
  s = ldir.st();
  s.reg[B] = 0;
  s.reg[C] = 2;
  s.reg[H] = 0x20;
  s.reg[D] = 0x30;
  ldir.set(s);
  CHECK(ldir.step() == 21);
  CHECK(ldir.step() == 16);
  CHECK(ldir.st().reg[C] == 0);
}

void testArithmeticAndFlags() {
  Rig add({0x3E, 0x7F, 0xC6, 0x01});  // LD A,7F ; ADD A,1 -> 80, S H P/V
  add.step();
  add.step();
  Z80State s = add.st();
  CHECK(s.reg[A] == 0x80);
  CHECK((s.reg[F] & 0x80) != 0);  // S
  CHECK((s.reg[F] & 0x10) != 0);  // H
  CHECK((s.reg[F] & 0x04) != 0);  // overflow
  CHECK((s.reg[F] & 0x02) == 0);  // N
  CHECK((s.reg[F] & 0x01) == 0);  // C

  Rig sub({0x3E, 0x00, 0xD6, 0x01});  // 0 - 1 = FF with C and N
  sub.step();
  sub.step();
  s = sub.st();
  CHECK(s.reg[A] == 0xFF);
  CHECK((s.reg[F] & 0x01) != 0);
  CHECK((s.reg[F] & 0x02) != 0);

  Rig daa({0x3E, 0x15, 0xC6, 0x27, 0x27});  // 15 + 27 = 3C -> DAA -> 42
  daa.step();
  daa.step();
  daa.step();
  CHECK(daa.st().reg[A] == 0x42);

  Rig cp({0x3E, 0x10, 0xFE, 0x28});  // CP takes X/Y from the operand
  cp.step();
  cp.step();
  s = cp.st();
  CHECK(s.reg[A] == 0x10);
  CHECK((s.reg[F] & 0x28) == 0x28);

  Rig bit7({0xCB, 0x7C});  // BIT 7,H with H = 0x80 -> S set, Z clear
  s = bit7.st();
  s.reg[H] = 0x80;
  bit7.set(s);
  bit7.step();
  s = bit7.st();
  CHECK((s.reg[F] & 0x80) != 0);
  CHECK((s.reg[F] & 0x40) == 0);
  CHECK((s.reg[F] & 0x10) != 0);  // H always set by BIT
}

void testMemoryAndStack() {
  Rig r({0x21, 0x00, 0x20, 0x36, 0xAB, 0x7E});  // LD HL,2000 ; LD (HL),AB ; LD A,(HL)
  r.step();
  r.step();
  r.step();
  CHECK(r.bus.mem[0x2000] == 0xAB);
  CHECK(r.st().reg[A] == 0xAB);

  Rig st({0x01, 0x34, 0x12, 0xC5, 0xD1});  // LD BC,1234 ; PUSH BC ; POP DE
  st.step();
  st.step();
  st.step();
  Z80State s = st.st();
  CHECK(s.reg[D] == 0x12 && s.reg[E] == 0x34);
  CHECK(s.SP == 0xF000);

  Rig call({0xCD, 0x00, 0x02});
  call.bus.mem[0x0200] = 0xC9;  // RET
  call.step();
  CHECK(call.cpu.getPC() == 0x0200);
  call.step();
  CHECK(call.cpu.getPC() == 0x0103);
}

void testIndexRegisters() {
  Rig ix({0xDD, 0x21, 0x00, 0x30, 0xDD, 0x36, 0x02, 0x77, 0xDD, 0x7E, 0x02});
  ix.step();  // LD IX,3000
  ix.step();  // LD (IX+2),77
  ix.step();  // LD A,(IX+2)
  CHECK(ix.bus.mem[0x3002] == 0x77);
  CHECK(ix.st().reg[A] == 0x77);

  Rig neg({0xDD, 0x21, 0x00, 0x30, 0xDD, 0x36, 0xFE, 0x55});  // (IX-2)
  neg.step();
  neg.step();
  CHECK(neg.bus.mem[0x2FFE] == 0x55);

  Rig ixh({0xDD, 0x26, 0xAB, 0xDD, 0x2E, 0xCD});  // undocumented LD IXH/IXL,n
  ixh.step();
  ixh.step();
  CHECK(ixh.st().IX == 0xABCD);
  CHECK(ixh.st().reg[H] == 0 && ixh.st().reg[L] == 0);  // real HL untouched

  // DDCB undocumented: RLC (IX+0) also copies the result into B.
  Rig ddcb({0xDD, 0x21, 0x00, 0x30, 0xDD, 0xCB, 0x00, 0x00});
  ddcb.bus.mem[0x3000] = 0x81;
  ddcb.step();
  ddcb.step();
  CHECK(ddcb.bus.mem[0x3000] == 0x03);
  CHECK(ddcb.st().reg[B] == 0x03);
}

void testIO() {
  Rig io({0x3E, 0x12, 0xDB, 0x34, 0xD3, 0x56});  // LD A,12 ; IN A,(34) ; OUT (56),A
  io.step();
  io.step();
  CHECK(io.bus.lastPortRead == 0x1234);  // A supplies the high byte
  CHECK(io.st().reg[A] == 0x5A);
  io.step();
  CHECK(io.bus.lastPortWrite == 0x5A56);
  CHECK(io.bus.lastPortValue == 0x5A);

  Rig inc({0xED, 0x78});  // IN A,(C) with BC = 0x00BE
  Z80State s = inc.st();
  s.reg[C] = 0xBE;
  inc.set(s);
  inc.step();
  CHECK(inc.bus.lastPortRead == 0x00BE);
  CHECK(inc.st().reg[A] == 0x5A);
}

void testInterrupts() {
  // IM 1: /INT asserted with IFF1 set jumps to 0x38 in 13 T-states.
  Rig im1({0x00});
  Z80State s = im1.st();
  s.IFF1 = s.IFF2 = true;
  s.IM = 1;
  im1.set(s);
  im1.cpu.setIRQ(true);
  CHECK(im1.step() == 13);
  CHECK(im1.cpu.getPC() == 0x0038);
  CHECK(!im1.st().IFF1);
  CHECK(im1.bus.mem[0xEFFE] == 0x00 && im1.bus.mem[0xEFFF] == 0x01);  // push 0100

  // Interrupts stay off when IFF1 is clear.
  Rig masked({0x00});
  masked.cpu.setIRQ(true);
  CHECK(masked.step() == 4);

  // EI delays acceptance by one instruction.
  Rig ei({0xFB, 0x00, 0x00});
  s = ei.st();
  s.IM = 1;
  ei.set(s);
  ei.cpu.setIRQ(true);
  CHECK(ei.step() == 4);  // EI
  CHECK(ei.step() == 4);  // NOP runs before the interrupt is taken
  CHECK(ei.step() == 13);
  CHECK(ei.cpu.getPC() == 0x0038);

  // IM 2: vector read from (I << 8 | data).
  Rig im2({0x00});
  s = im2.st();
  s.IFF1 = s.IFF2 = true;
  s.IM = 2;
  s.I = 0x40;
  im2.set(s);
  im2.bus.mem[0x40FF] = 0x00;
  im2.bus.mem[0x4100] = 0x80;
  im2.cpu.setIRQ(true, 0xFF);
  CHECK(im2.step() == 19);
  CHECK(im2.cpu.getPC() == 0x8000);

  // NMI: vector 0x66, 11 T-states, IFF2 keeps the old IFF1.
  Rig nmi({0x00});
  s = nmi.st();
  s.IFF1 = s.IFF2 = true;
  nmi.set(s);
  nmi.cpu.requestNMI();
  CHECK(nmi.step() == 11);
  CHECK(nmi.cpu.getPC() == 0x0066);
  CHECK(!nmi.st().IFF1 && nmi.st().IFF2);

  // HALT: burns 4 T-states per step until an interrupt wakes it up.
  Rig halt({0x76});
  s = halt.st();
  s.IFF1 = s.IFF2 = true;
  s.IM = 1;
  halt.set(s);
  CHECK(halt.step() == 4);
  CHECK(halt.step() == 4);
  CHECK(halt.cpu.getPC() == 0x0101);
  halt.cpu.setIRQ(true);
  CHECK(halt.step() == 13);
  CHECK(halt.bus.mem[0xEFFE] == 0x01 && halt.bus.mem[0xEFFF] == 0x01);
}

void testStateRoundTrip() {
  Rig r({0x3E, 0x42});
  r.step();
  Z80State a = r.st();
  nlohmann::json j = a;
  Z80State b = j.get<Z80State>();
  CHECK(b.reg == a.reg && b.PC == a.PC && b.SP == a.SP && b.R == a.R);
}

}  // namespace

int main() {
  testTimings();
  testArithmeticAndFlags();
  testMemoryAndStack();
  testIndexRegisters();
  testIO();
  testInterrupts();
  testStateRoundTrip();
  if (failures == 0) std::printf("z80_core: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
