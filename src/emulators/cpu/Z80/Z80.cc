#include "emulators/cpu/Z80/Z80.hh"

#include <utility>

namespace {

constexpr uint8_t FS = 0x80, FZ = 0x40, FY = 0x20, FH = 0x10;
constexpr uint8_t FX = 0x08, FP = 0x04, FN = 0x02, FC = 0x01;

/// Parity table: FP when the byte has an even number of set bits, else 0.
const std::array<uint8_t, 256> kParity = [] {
  std::array<uint8_t, 256> t{};
  for (int i = 0; i < 256; i++) {
    int bits = 0;
    for (int j = 0; j < 8; j++) bits ^= (i >> j) & 1;
    t[static_cast<size_t>(i)] = bits == 0 ? FP : 0;
  }
  return t;
}();

inline uint8_t szxy(uint8_t v) {
  return static_cast<uint8_t>((v & (FS | FY | FX)) | (v == 0 ? FZ : 0));
}
inline uint8_t szxyp(uint8_t v) {
  return static_cast<uint8_t>(szxy(v) | kParity[v]);
}
inline uint8_t u8(int v) { return static_cast<uint8_t>(v); }
inline uint16_t u16(int v) { return static_cast<uint16_t>(v); }

}  // namespace

// ---------------------------------------------------------------------------
// Construction, reset, state
// ---------------------------------------------------------------------------

Z80::Z80(Z80Bus *bus) : bus_(bus) { reset(); }

void Z80::reset() {
  r_.fill(0);
  alt_.fill(0);
  r_[rA] = 0xFF;
  r_[rF] = 0xFF;
  ix_ = iy_ = 0xFFFF;
  sp_ = 0xFFFF;
  pc_ = 0;
  wz_ = 0;
  i_ = rr_ = im_ = 0;
  iff1_ = iff2_ = halted_ = eiDelay_ = false;
  nmiPending_ = irqLine_ = ldAIR_ = false;
  irqData_ = 0xFF;
  q_ = lastQ_ = 0;
  idx_ = 0;
}

Z80State Z80::getState() const {
  Z80State s;
  s.reg = r_;
  s.alt = alt_;
  s.IX = ix_;
  s.IY = iy_;
  s.SP = sp_;
  s.PC = pc_;
  s.WZ = wz_;
  s.I = i_;
  s.R = rr_;
  s.IM = im_;
  s.IFF1 = iff1_;
  s.IFF2 = iff2_;
  s.halted = halted_;
  s.eiDelay = eiDelay_;
  s.nmiPending = nmiPending_;
  s.irqLine = irqLine_;
  s.irqData = irqData_;
  s.q = q_;
  s.ldAIR = ldAIR_;
  return s;
}

void Z80::setState(const Z80State &s) {
  r_ = s.reg;
  alt_ = s.alt;
  ix_ = s.IX;
  iy_ = s.IY;
  sp_ = s.SP;
  pc_ = s.PC;
  wz_ = s.WZ;
  i_ = s.I;
  rr_ = s.R;
  im_ = s.IM;
  iff1_ = s.IFF1;
  iff2_ = s.IFF2;
  halted_ = s.halted;
  eiDelay_ = s.eiDelay;
  nmiPending_ = s.nmiPending;
  irqLine_ = s.irqLine;
  irqData_ = s.irqData;
  q_ = s.q;
  ldAIR_ = s.ldAIR;
}

void Z80::requestNMI() { nmiPending_ = true; }

void Z80::setIRQ(bool level, uint8_t dataBus) {
  irqLine_ = level;
  irqData_ = dataBus;
}

// ---------------------------------------------------------------------------
// Register helpers
// ---------------------------------------------------------------------------

void Z80::setHL(uint16_t v) {
  r_[rH] = u8(v >> 8);
  r_[rL] = u8(v & 0xFF);
}
void Z80::setBC(uint16_t v) {
  r_[rB] = u8(v >> 8);
  r_[rC] = u8(v & 0xFF);
}
void Z80::setDE(uint16_t v) {
  r_[rD] = u8(v >> 8);
  r_[rE] = u8(v & 0xFF);
}

uint8_t Z80::getR8(int i) const {
  if (idx_ != 0 && (i == rH || i == rL)) {
    uint16_t v = idx_ == 1 ? ix_ : iy_;
    return i == rH ? u8(v >> 8) : u8(v & 0xFF);
  }
  return r_[static_cast<size_t>(i)];
}

void Z80::setR8(int i, uint8_t v) {
  if (idx_ != 0 && (i == rH || i == rL)) {
    uint16_t &ixy = indexReg();
    ixy = i == rH ? u16((ixy & 0x00FF) | (v << 8)) : u16((ixy & 0xFF00) | v);
    return;
  }
  r_[static_cast<size_t>(i)] = v;
}

uint16_t Z80::rpGet(int p) const {
  switch (p) {
    case 0: return pairBC();
    case 1: return pairDE();
    case 2: return idx_ == 0 ? hl() : (idx_ == 1 ? ix_ : iy_);
    default: return sp_;
  }
}

void Z80::rpSet(int p, uint16_t v) {
  switch (p) {
    case 0: setBC(v); break;
    case 1: setDE(v); break;
    case 2:
      if (idx_ == 0) {
        setHL(v);
      } else {
        indexReg() = v;
      }
      break;
    default: sp_ = v; break;
  }
}

uint16_t Z80::rp2Get(int p) const {
  if (p == 3) return u16((r_[rA] << 8) | r_[rF]);
  return rpGet(p);
}

void Z80::rp2Set(int p, uint16_t v) {
  if (p == 3) {
    r_[rA] = u8(v >> 8);
    setF(u8(v & 0xFF));
  } else {
    rpSet(p, v);
  }
}

bool Z80::cond(int cc) const {
  uint8_t f = r_[rF];
  bool t = false;
  switch (cc >> 1) {
    case 0: t = (f & FZ) != 0; break;
    case 1: t = (f & FC) != 0; break;
    case 2: t = (f & FP) != 0; break;
    default: t = (f & FS) != 0; break;
  }
  return (cc & 1) ? t : !t;
}

// ---------------------------------------------------------------------------
// Memory / fetch / stack helpers
// ---------------------------------------------------------------------------

uint8_t Z80::read(uint32_t &cycles, uint16_t address) {
  cycles += 3;
  return bus_->read(address);
}

void Z80::write(uint32_t &cycles, uint16_t address, uint8_t value) {
  cycles += 3;
  bus_->write(address, value);
}

uint8_t Z80::fetchOp(uint32_t &c) {
  c += 4;
  rr_ = u8((rr_ & 0x80) | ((rr_ + 1) & 0x7F));
  return bus_->read(pc_++);
}

uint8_t Z80::fetch8(uint32_t &c) {
  c += 3;
  return bus_->read(pc_++);
}

uint16_t Z80::fetch16(uint32_t &c) {
  uint8_t lo = fetch8(c);
  uint8_t hi = fetch8(c);
  return u16((hi << 8) | lo);
}

void Z80::push(uint32_t &c, uint16_t v) {
  sp_--;
  write(c, sp_, u8(v >> 8));
  sp_--;
  write(c, sp_, u8(v & 0xFF));
}

uint16_t Z80::pop(uint32_t &c) {
  uint8_t lo = read(c, sp_++);
  uint8_t hi = read(c, sp_++);
  return u16((hi << 8) | lo);
}

uint8_t Z80::in(uint32_t &c, uint16_t port) {
  c += 4;
  return bus_->readPort(port);
}

void Z80::out(uint32_t &c, uint16_t port, uint8_t v) {
  c += 4;
  bus_->writePort(port, v);
}

uint16_t Z80::indexAddr(uint32_t &c, uint32_t internal) {
  int8_t d = static_cast<int8_t>(fetch8(c));
  c += internal;
  uint16_t a = u16(indexReg() + d);
  wz_ = a;
  return a;
}

uint16_t Z80::memOperand(uint32_t &c) {
  return idx_ != 0 ? indexAddr(c, 5) : hl();
}

// ---------------------------------------------------------------------------
// ALU
// ---------------------------------------------------------------------------

uint8_t Z80::add8(uint8_t a, uint8_t v, uint8_t carry) {
  int res = a + v + carry;
  uint8_t r = u8(res);
  uint8_t f = szxy(r);
  if ((a ^ v ^ r) & 0x10) f |= FH;
  if ((a ^ r) & (v ^ r) & 0x80) f |= FP;
  if (res & 0x100) f |= FC;
  setF(f);
  return r;
}

uint8_t Z80::sub8(uint8_t a, uint8_t v, uint8_t carry) {
  int res = a - v - carry;
  uint8_t r = u8(res);
  uint8_t f = u8(szxy(r) | FN);
  if ((a ^ v ^ r) & 0x10) f |= FH;
  if ((a ^ v) & (a ^ r) & 0x80) f |= FP;
  if (res & 0x100) f |= FC;
  setF(f);
  return r;
}

void Z80::alu(int op, uint8_t v) {
  uint8_t a = r_[rA];
  switch (op) {
    case 0: r_[rA] = add8(a, v, 0); break;
    case 1: r_[rA] = add8(a, v, r_[rF] & FC); break;
    case 2: r_[rA] = sub8(a, v, 0); break;
    case 3: r_[rA] = sub8(a, v, r_[rF] & FC); break;
    case 4:
      r_[rA] = a & v;
      setF(u8(szxyp(r_[rA]) | FH));
      break;
    case 5:
      r_[rA] = a ^ v;
      setF(szxyp(r_[rA]));
      break;
    case 6:
      r_[rA] = a | v;
      setF(szxyp(r_[rA]));
      break;
    default:  // CP: X/Y come from the operand, not the result.
      sub8(a, v, 0);
      setF(u8((r_[rF] & ~(FX | FY)) | (v & (FX | FY))));
      break;
  }
}

uint8_t Z80::inc8(uint8_t v) {
  uint8_t r = u8(v + 1);
  uint8_t f = u8((r_[rF] & FC) | szxy(r));
  if ((v & 0x0F) == 0x0F) f |= FH;
  if (v == 0x7F) f |= FP;
  setF(f);
  return r;
}

uint8_t Z80::dec8(uint8_t v) {
  uint8_t r = u8(v - 1);
  uint8_t f = u8((r_[rF] & FC) | szxy(r) | FN);
  if ((v & 0x0F) == 0x00) f |= FH;
  if (v == 0x80) f |= FP;
  setF(f);
  return r;
}

uint16_t Z80::add16(uint16_t a, uint16_t b) {
  uint32_t r = static_cast<uint32_t>(a) + b;
  uint8_t f = u8(r_[rF] & (FS | FZ | FP));
  f |= u8((r >> 8) & (FY | FX));
  if (((a ^ b ^ r) >> 8) & 0x10) f |= FH;
  if (r & 0x10000) f |= FC;
  setF(f);
  wz_ = u16(a + 1);
  return u16(r & 0xFFFF);
}

void Z80::adc16(uint16_t b) {
  uint16_t a = hl();
  uint32_t r = static_cast<uint32_t>(a) + b + (r_[rF] & FC);
  uint16_t r16 = u16(r & 0xFFFF);
  uint8_t f = u8((r >> 8) & (FS | FY | FX));
  if (r16 == 0) f |= FZ;
  if (((a ^ b ^ r) >> 8) & 0x10) f |= FH;
  if ((a ^ r) & (b ^ r) & 0x8000) f |= FP;
  if (r & 0x10000) f |= FC;
  setF(f);
  wz_ = u16(a + 1);
  setHL(r16);
}

void Z80::sbc16(uint16_t b) {
  uint16_t a = hl();
  uint32_t r = static_cast<uint32_t>(a) - b - (r_[rF] & FC);
  uint16_t r16 = u16(r & 0xFFFF);
  uint8_t f = u8(((r >> 8) & (FS | FY | FX)) | FN);
  if (r16 == 0) f |= FZ;
  if (((a ^ b ^ r) >> 8) & 0x10) f |= FH;
  if ((a ^ b) & (a ^ r) & 0x8000) f |= FP;
  if (r & 0x10000) f |= FC;
  setF(f);
  wz_ = u16(a + 1);
  setHL(r16);
}

uint8_t Z80::rot(int y, uint8_t v) {
  uint8_t cf = 0, r = 0;
  switch (y) {
    case 0: cf = v >> 7; r = u8((v << 1) | cf); break;                 // RLC
    case 1: cf = v & 1; r = u8((v >> 1) | (cf << 7)); break;          // RRC
    case 2: cf = v >> 7; r = u8((v << 1) | (r_[rF] & FC)); break;     // RL
    case 3: cf = v & 1; r = u8((v >> 1) | ((r_[rF] & FC) << 7)); break;  // RR
    case 4: cf = v >> 7; r = u8(v << 1); break;                       // SLA
    case 5: cf = v & 1; r = u8((v >> 1) | (v & 0x80)); break;         // SRA
    case 6: cf = v >> 7; r = u8((v << 1) | 1); break;                 // SLL
    default: cf = v & 1; r = u8(v >> 1); break;                       // SRL
  }
  setF(u8(szxyp(r) | cf));
  return r;
}

void Z80::bit(int n, uint8_t v, uint8_t xySource) {
  uint8_t t = u8(v & (1 << n));
  uint8_t f = u8((r_[rF] & FC) | FH | (t & FS) | (xySource & (FX | FY)));
  if (t == 0) f |= FZ | FP;
  setF(f);
}

void Z80::daa() {
  uint8_t a = r_[rA];
  uint8_t f = r_[rF];
  uint8_t corr = 0;
  uint8_t cf = f & FC;
  if ((f & FH) || (a & 0x0F) > 9) corr |= 0x06;
  if (cf || a > 0x99) {
    corr |= 0x60;
    cf = FC;
  }
  uint8_t res = (f & FN) ? u8(a - corr) : u8(a + corr);
  uint8_t hf;
  if (f & FN) {
    hf = ((f & FH) && (a & 0x0F) < 6) ? FH : 0;
  } else {
    hf = (a & 0x0F) > 9 ? FH : 0;
  }
  r_[rA] = res;
  setF(u8(szxyp(res) | (f & FN) | hf | cf));
}

// ---------------------------------------------------------------------------
// Execution
// ---------------------------------------------------------------------------

uint32_t Z80::execute() {
  uint32_t c = 0;
  bool allowInt = !eiDelay_;
  bool prevLdAIR = ldAIR_;
  eiDelay_ = false;
  ldAIR_ = false;
  lastQ_ = q_;
  q_ = 0;

  if (nmiPending_) {
    nmiPending_ = false;
    halted_ = false;
    iff2_ = iff1_;
    iff1_ = false;
    rr_ = u8((rr_ & 0x80) | ((rr_ + 1) & 0x7F));
    c = 5;
    push(c, pc_);
    pc_ = 0x66;
    wz_ = pc_;
    return c;
  }

  if (irqLine_ && iff1_ && allowInt) {
    halted_ = false;
    iff1_ = iff2_ = false;
    rr_ = u8((rr_ & 0x80) | ((rr_ + 1) & 0x7F));
    if (prevLdAIR) r_[rF] = u8(r_[rF] & ~FP);
    c = 7;
    push(c, pc_);
    if (im_ == 2) {
      uint16_t vec = u16((i_ << 8) | irqData_);
      uint8_t lo = read(c, vec);
      uint8_t hi = read(c, u16(vec + 1));
      pc_ = u16((hi << 8) | lo);
    } else if (im_ == 0) {
      pc_ = u16(irqData_ & 0x38);  // Device supplies an RST opcode.
    } else {
      pc_ = 0x38;
    }
    wz_ = pc_;
    return c;
  }

  if (halted_) {
    rr_ = u8((rr_ & 0x80) | ((rr_ + 1) & 0x7F));
    return 4;
  }

  idx_ = 0;
  uint8_t op = fetchOp(c);
  while (op == 0xDD || op == 0xFD) {
    idx_ = op == 0xDD ? 1 : 2;
    op = fetchOp(c);
  }

  if (op == 0xCB) {
    if (idx_ != 0) {
      executeIndexedCB(c);
    } else {
      executeCB(c);
    }
  } else if (op == 0xED) {
    idx_ = 0;
    executeED(c);
  } else {
    executeMain(op, c);
  }
  return c;
}

void Z80::executeMain(uint8_t op, uint32_t &c) {
  const int x = op >> 6;
  const int y = (op >> 3) & 7;
  const int z = op & 7;
  const int p = y >> 1;
  const int q = y & 1;

  switch (x) {
    case 0:
      switch (z) {
        case 0:
          switch (y) {
            case 0: break;  // NOP
            case 1:         // EX AF,AF'
              std::swap(r_[rA], alt_[rA]);
              std::swap(r_[rF], alt_[rF]);
              break;
            case 2: {  // DJNZ d
              int8_t d = static_cast<int8_t>(fetch8(c));
              c += 1;
              if (--r_[rB] != 0) {
                c += 5;
                pc_ = u16(pc_ + d);
                wz_ = pc_;
              }
              break;
            }
            case 3: {  // JR d
              int8_t d = static_cast<int8_t>(fetch8(c));
              c += 5;
              pc_ = u16(pc_ + d);
              wz_ = pc_;
              break;
            }
            default: {  // JR cc,d (cc = NZ, Z, NC, C)
              int8_t d = static_cast<int8_t>(fetch8(c));
              if (cond(y - 4)) {
                c += 5;
                pc_ = u16(pc_ + d);
                wz_ = pc_;
              }
              break;
            }
          }
          break;
        case 1:
          if (q == 0) {  // LD rp,nn
            rpSet(p, fetch16(c));
          } else {  // ADD HL,rp
            c += 7;
            rpSet(2, add16(rpGet(2), rpGet(p)));
          }
          break;
        case 2:
          switch (p) {
            case 0:
            case 1: {  // LD (BC/DE),A and LD A,(BC/DE)
              uint16_t addr = p == 0 ? pairBC() : pairDE();
              if (q == 0) {
                write(c, addr, r_[rA]);
                wz_ = u16(((addr + 1) & 0xFF) | (r_[rA] << 8));
              } else {
                r_[rA] = read(c, addr);
                wz_ = u16(addr + 1);
              }
              break;
            }
            case 2: {  // LD (nn),HL and LD HL,(nn)
              uint16_t nn = fetch16(c);
              if (q == 0) {
                uint16_t v = rpGet(2);
                write(c, nn, u8(v & 0xFF));
                write(c, u16(nn + 1), u8(v >> 8));
              } else {
                uint8_t lo = read(c, nn);
                uint8_t hi = read(c, u16(nn + 1));
                rpSet(2, u16((hi << 8) | lo));
              }
              wz_ = u16(nn + 1);
              break;
            }
            default: {  // LD (nn),A and LD A,(nn)
              uint16_t nn = fetch16(c);
              if (q == 0) {
                write(c, nn, r_[rA]);
                wz_ = u16(((nn + 1) & 0xFF) | (r_[rA] << 8));
              } else {
                r_[rA] = read(c, nn);
                wz_ = u16(nn + 1);
              }
              break;
            }
          }
          break;
        case 3:  // INC/DEC rp
          c += 2;
          rpSet(p, u16(rpGet(p) + (q == 0 ? 1 : -1)));
          break;
        case 4:    // INC r / INC (HL)
        case 5: {  // DEC r / DEC (HL)
          if (y == 6) {
            uint16_t addr = memOperand(c);
            uint8_t v = read(c, addr);
            c += 1;
            v = z == 4 ? inc8(v) : dec8(v);
            write(c, addr, v);
          } else {
            uint8_t v = getR8(y);
            setR8(y, z == 4 ? inc8(v) : dec8(v));
          }
          break;
        }
        case 6:  // LD r,n / LD (HL),n
          if (y == 6) {
            uint16_t addr;
            uint8_t n;
            if (idx_ != 0) {
              addr = indexAddr(c, 0);
              n = fetch8(c);
              c += 2;
            } else {
              n = fetch8(c);
              addr = hl();
            }
            write(c, addr, n);
          } else {
            setR8(y, fetch8(c));
          }
          break;
        default: {  // z == 7
          uint8_t a = r_[rA];
          uint8_t keep = u8(r_[rF] & (FS | FZ | FP));
          switch (y) {
            case 0: {  // RLCA
              uint8_t cf = a >> 7;
              a = u8((a << 1) | cf);
              r_[rA] = a;
              setF(u8(keep | (a & (FX | FY)) | cf));
              break;
            }
            case 1: {  // RRCA
              uint8_t cf = a & 1;
              a = u8((a >> 1) | (cf << 7));
              r_[rA] = a;
              setF(u8(keep | (a & (FX | FY)) | cf));
              break;
            }
            case 2: {  // RLA
              uint8_t cf = a >> 7;
              a = u8((a << 1) | (r_[rF] & FC));
              r_[rA] = a;
              setF(u8(keep | (a & (FX | FY)) | cf));
              break;
            }
            case 3: {  // RRA
              uint8_t cf = a & 1;
              a = u8((a >> 1) | ((r_[rF] & FC) << 7));
              r_[rA] = a;
              setF(u8(keep | (a & (FX | FY)) | cf));
              break;
            }
            case 4: daa(); break;
            case 5:  // CPL
              r_[rA] = u8(~a);
              setF(u8((r_[rF] & (FS | FZ | FP | FC)) | FH | FN |
                      (r_[rA] & (FX | FY))));
              break;
            case 6:  // SCF
              setF(u8(keep | (((lastQ_ ^ r_[rF]) | a) & (FX | FY)) | FC));
              break;
            default: {  // CCF
              uint8_t oldC = r_[rF] & FC;
              setF(u8(keep | (oldC ? FH : 0) |
                      (((lastQ_ ^ r_[rF]) | a) & (FX | FY)) | (oldC ? 0 : FC)));
              break;
            }
          }
          break;
        }
      }
      break;

    case 1:
      if (op == 0x76) {  // HALT
        halted_ = true;
      } else if (y == 6) {  // LD (HL),r  (r is always the real register)
        uint16_t addr = memOperand(c);
        write(c, addr, r_[static_cast<size_t>(z)]);
      } else if (z == 6) {  // LD r,(HL)
        uint16_t addr = memOperand(c);
        r_[static_cast<size_t>(y)] = read(c, addr);
      } else {
        setR8(y, getR8(z));
      }
      break;

    case 2:
      if (z == 6) {
        uint16_t addr = memOperand(c);
        alu(y, read(c, addr));
      } else {
        alu(y, getR8(z));
      }
      break;

    default:  // x == 3
      switch (z) {
        case 0:  // RET cc
          c += 1;
          if (cond(y)) {
            pc_ = pop(c);
            wz_ = pc_;
          }
          break;
        case 1:
          if (q == 0) {  // POP rp2
            rp2Set(p, pop(c));
          } else {
            switch (p) {
              case 0:  // RET
                pc_ = pop(c);
                wz_ = pc_;
                break;
              case 1:  // EXX
                for (size_t i = 0; i < 6; i++) std::swap(r_[i], alt_[i]);
                break;
              case 2:  // JP (HL)
                pc_ = rpGet(2);
                break;
              default:  // LD SP,HL
                c += 2;
                sp_ = rpGet(2);
                break;
            }
          }
          break;
        case 2: {  // JP cc,nn
          uint16_t nn = fetch16(c);
          wz_ = nn;
          if (cond(y)) pc_ = nn;
          break;
        }
        case 3:
          switch (y) {
            case 0:  // JP nn
              pc_ = fetch16(c);
              wz_ = pc_;
              break;
            case 2: {  // OUT (n),A
              uint8_t n = fetch8(c);
              uint16_t port = u16((r_[rA] << 8) | n);
              out(c, port, r_[rA]);
              wz_ = u16(((n + 1) & 0xFF) | (r_[rA] << 8));
              break;
            }
            case 3: {  // IN A,(n)
              uint8_t n = fetch8(c);
              uint16_t port = u16((r_[rA] << 8) | n);
              r_[rA] = in(c, port);
              wz_ = u16(port + 1);
              break;
            }
            case 4: {  // EX (SP),HL
              uint8_t lo = read(c, sp_);
              uint8_t hi = read(c, u16(sp_ + 1));
              c += 1;
              uint16_t old = rpGet(2);
              write(c, u16(sp_ + 1), u8(old >> 8));
              write(c, sp_, u8(old & 0xFF));
              c += 2;
              rpSet(2, u16((hi << 8) | lo));
              wz_ = u16((hi << 8) | lo);
              break;
            }
            case 5: {  // EX DE,HL (never affected by DD/FD)
              std::swap(r_[rD], r_[rH]);
              std::swap(r_[rE], r_[rL]);
              break;
            }
            case 6:  // DI
              iff1_ = iff2_ = false;
              break;
            default:  // EI
              iff1_ = iff2_ = true;
              eiDelay_ = true;
              break;
          }
          break;
        case 4: {  // CALL cc,nn
          uint16_t nn = fetch16(c);
          wz_ = nn;
          if (cond(y)) {
            c += 1;
            push(c, pc_);
            pc_ = nn;
          }
          break;
        }
        case 5:
          if (q == 0) {  // PUSH rp2
            c += 1;
            push(c, rp2Get(p));
          } else {  // CALL nn (p == 0; other p are prefixes handled earlier)
            uint16_t nn = fetch16(c);
            wz_ = nn;
            c += 1;
            push(c, pc_);
            pc_ = nn;
          }
          break;
        case 6:  // ALU A,n
          alu(y, fetch8(c));
          break;
        default:  // RST y*8
          c += 1;
          push(c, pc_);
          pc_ = u16(y * 8);
          wz_ = pc_;
          break;
      }
      break;
  }
}

void Z80::executeCB(uint32_t &c) {
  uint8_t op = fetchOp(c);
  const int x = op >> 6;
  const int y = (op >> 3) & 7;
  const int z = op & 7;

  if (z == 6) {
    uint16_t addr = hl();
    uint8_t v = read(c, addr);
    c += 1;
    switch (x) {
      case 0: write(c, addr, rot(y, v)); break;
      case 1: bit(y, v, u8(wz_ >> 8)); break;
      case 2: write(c, addr, u8(v & ~(1 << y))); break;
      default: write(c, addr, u8(v | (1 << y))); break;
    }
    return;
  }

  uint8_t &reg = r_[static_cast<size_t>(z)];
  switch (x) {
    case 0: reg = rot(y, reg); break;
    case 1: bit(y, reg, reg); break;
    case 2: reg = u8(reg & ~(1 << y)); break;
    default: reg = u8(reg | (1 << y)); break;
  }
}

void Z80::executeIndexedCB(uint32_t &c) {
  // DD CB d op: the displacement and the final opcode are plain operand reads.
  int8_t d = static_cast<int8_t>(fetch8(c));
  uint8_t op = fetch8(c);
  c += 2;
  const int x = op >> 6;
  const int y = (op >> 3) & 7;
  const int z = op & 7;

  uint16_t addr = u16(indexReg() + d);
  wz_ = addr;
  uint8_t v = read(c, addr);
  c += 1;

  uint8_t res = v;
  switch (x) {
    case 0: res = rot(y, v); break;
    case 1:  // BIT n,(IX+d): every z behaves the same; X/Y from address high.
      bit(y, v, u8(addr >> 8));
      return;
    case 2: res = u8(v & ~(1 << y)); break;
    default: res = u8(v | (1 << y)); break;
  }
  write(c, addr, res);
  // Undocumented: the result is also copied into a register (z != 6).
  if (z != 6) r_[static_cast<size_t>(z)] = res;
}

void Z80::executeED(uint32_t &c) {
  uint8_t op = fetchOp(c);
  const int x = op >> 6;
  const int y = (op >> 3) & 7;
  const int z = op & 7;
  const int p = y >> 1;
  const int q = y & 1;

  if (x == 1) {
    switch (z) {
      case 0: {  // IN r,(C)  (y == 6: only sets flags)
        uint16_t port = pairBC();
        uint8_t v = in(c, port);
        wz_ = u16(port + 1);
        if (y != 6) r_[static_cast<size_t>(y)] = v;
        setF(u8((r_[rF] & FC) | szxyp(v)));
        break;
      }
      case 1: {  // OUT (C),r  (y == 6: outputs 0)
        uint16_t port = pairBC();
        out(c, port, y == 6 ? 0 : r_[static_cast<size_t>(y)]);
        wz_ = u16(port + 1);
        break;
      }
      case 2:  // SBC/ADC HL,rp
        c += 7;
        if (q == 0) {
          sbc16(rpGet(p));
        } else {
          adc16(rpGet(p));
        }
        break;
      case 3: {  // LD (nn),rp / LD rp,(nn)
        uint16_t nn = fetch16(c);
        if (q == 0) {
          uint16_t v = rpGet(p);
          write(c, nn, u8(v & 0xFF));
          write(c, u16(nn + 1), u8(v >> 8));
        } else {
          uint8_t lo = read(c, nn);
          uint8_t hi = read(c, u16(nn + 1));
          rpSet(p, u16((hi << 8) | lo));
        }
        wz_ = u16(nn + 1);
        break;
      }
      case 4:  // NEG (and its mirrors)
        r_[rA] = sub8(0, r_[rA], 0);
        break;
      case 5:  // RETN / RETI (and mirrors)
        pc_ = pop(c);
        wz_ = pc_;
        iff1_ = iff2_;
        break;
      case 6: {  // IM 0/1/2 (mirrors included)
        static const uint8_t modes[8] = {0, 0, 1, 2, 0, 0, 1, 2};
        im_ = modes[y];
        break;
      }
      default:
        switch (y) {
          case 0: c += 1; i_ = r_[rA]; break;  // LD I,A
          case 1: c += 1; rr_ = r_[rA]; break;  // LD R,A
          case 2:                               // LD A,I
          case 3: {                             // LD A,R
            c += 1;
            uint8_t v = y == 2 ? i_ : rr_;
            r_[rA] = v;
            setF(u8((r_[rF] & FC) | szxy(v) | (iff2_ ? FP : 0)));
            ldAIR_ = true;
            break;
          }
          case 4: {  // RRD
            uint16_t addr = hl();
            uint8_t v = read(c, addr);
            c += 4;
            uint8_t a = r_[rA];
            write(c, addr, u8((a << 4) | (v >> 4)));
            r_[rA] = u8((a & 0xF0) | (v & 0x0F));
            setF(u8((r_[rF] & FC) | szxyp(r_[rA])));
            wz_ = u16(addr + 1);
            break;
          }
          case 5: {  // RLD
            uint16_t addr = hl();
            uint8_t v = read(c, addr);
            c += 4;
            uint8_t a = r_[rA];
            write(c, addr, u8((v << 4) | (a & 0x0F)));
            r_[rA] = u8((a & 0xF0) | (v >> 4));
            setF(u8((r_[rF] & FC) | szxyp(r_[rA])));
            wz_ = u16(addr + 1);
            break;
          }
          default: break;  // ED 77 / ED 7F: NOP
        }
        break;
    }
    return;
  }

  if (x == 2 && y >= 4 && z <= 3) {
    switch (z) {
      case 0: blockTransfer(y, c); break;
      case 1: blockCompare(y, c); break;
      case 2: blockIn(y, c); break;
      default: blockOut(y, c); break;
    }
  }
  // Every other ED opcode is an 8 T-state NOP.
}

// ---------------------------------------------------------------------------
// Block instructions. y: 4 = xxI, 5 = xxD, 6 = xxIR, 7 = xxDR
// ---------------------------------------------------------------------------

void Z80::blockTransfer(int y, uint32_t &c) {
  const bool dec = (y & 1) != 0;
  const bool repeat = y >= 6;
  uint16_t src = hl();
  uint16_t dst = pairDE();
  uint8_t v = read(c, src);
  write(c, dst, v);
  c += 2;
  setHL(u16(src + (dec ? -1 : 1)));
  setDE(u16(dst + (dec ? -1 : 1)));
  uint16_t bc = u16(pairBC() - 1);
  setBC(bc);

  uint8_t n = u8(v + r_[rA]);
  uint8_t f = u8((r_[rF] & (FS | FZ | FC)) | (n & FX) | ((n & 2) ? FY : 0) |
                 (bc != 0 ? FP : 0));
  if (repeat && bc != 0) {
    c += 5;
    pc_ = u16(pc_ - 2);
    wz_ = u16(pc_ + 1);
    f = u8((f & ~(FX | FY)) | ((pc_ >> 8) & (FX | FY)));
  }
  setF(f);
}

void Z80::blockCompare(int y, uint32_t &c) {
  const bool dec = (y & 1) != 0;
  const bool repeat = y >= 6;
  uint16_t addr = hl();
  uint8_t v = read(c, addr);
  c += 5;
  uint8_t a = r_[rA];
  uint8_t res = u8(a - v);
  bool hf = ((a ^ v ^ res) & 0x10) != 0;
  setHL(u16(addr + (dec ? -1 : 1)));
  uint16_t bc = u16(pairBC() - 1);
  setBC(bc);
  wz_ = u16(wz_ + (dec ? -1 : 1));

  uint8_t n = u8(res - (hf ? 1 : 0));
  uint8_t f = u8((r_[rF] & FC) | (res & FS) | (res == 0 ? FZ : 0) |
                 (hf ? FH : 0) | FN | (bc != 0 ? FP : 0) | (n & FX) |
                 ((n & 2) ? FY : 0));
  if (repeat && bc != 0 && res != 0) {
    c += 5;
    pc_ = u16(pc_ - 2);
    wz_ = u16(pc_ + 1);
    f = u8((f & ~(FX | FY)) | ((pc_ >> 8) & (FX | FY)));
  }
  setF(f);
}

void Z80::blockIn(int y, uint32_t &c) {
  const bool dec = (y & 1) != 0;
  const bool repeat = y >= 6;
  c += 1;
  uint16_t port = pairBC();
  uint8_t v = in(c, port);
  uint16_t addr = hl();
  write(c, addr, v);
  r_[rB] = u8(r_[rB] - 1);
  wz_ = u16(port + (dec ? -1 : 1));
  setHL(u16(addr + (dec ? -1 : 1)));

  uint8_t b = r_[rB];
  uint32_t t = static_cast<uint32_t>((r_[rC] + (dec ? -1 : 1)) & 0xFF) + v;
  uint8_t f = szxy(b);
  if (v & 0x80) f |= FN;
  if (t > 0xFF) f |= FH | FC;
  f |= kParity[u8((t & 7) ^ b)];
  if (repeat && b != 0) {
    c += 5;
    pc_ = u16(pc_ - 2);
    f = u8((f & ~(FX | FY)) | ((pc_ >> 8) & (FX | FY)));
    if (f & FC) {
      if (v & 0x80) {
        if (kParity[u8((b - 1) & 7)] == 0) f ^= FP;
        f = u8((f & ~FH) | ((b & 0x0F) == 0 ? FH : 0));
      } else {
        if (kParity[u8((b + 1) & 7)] == 0) f ^= FP;
        f = u8((f & ~FH) | ((b & 0x0F) == 0x0F ? FH : 0));
      }
    } else if (kParity[u8(b & 7)] == 0) {
      f ^= FP;
    }
  }
  setF(f);
}

void Z80::blockOut(int y, uint32_t &c) {
  const bool dec = (y & 1) != 0;
  const bool repeat = y >= 6;
  c += 1;
  uint16_t addr = hl();
  uint8_t v = read(c, addr);
  r_[rB] = u8(r_[rB] - 1);
  uint16_t port = pairBC();
  out(c, port, v);
  wz_ = u16(port + (dec ? -1 : 1));
  setHL(u16(addr + (dec ? -1 : 1)));

  uint8_t b = r_[rB];
  uint32_t t = static_cast<uint32_t>(r_[rL]) + v;
  uint8_t f = szxy(b);
  if (v & 0x80) f |= FN;
  if (t > 0xFF) f |= FH | FC;
  f |= kParity[u8((t & 7) ^ b)];
  if (repeat && b != 0) {
    c += 5;
    pc_ = u16(pc_ - 2);
    f = u8((f & ~(FX | FY)) | ((pc_ >> 8) & (FX | FY)));
    if (f & FC) {
      if (v & 0x80) {
        if (kParity[u8((b - 1) & 7)] == 0) f ^= FP;
        f = u8((f & ~FH) | ((b & 0x0F) == 0 ? FH : 0));
      } else {
        if (kParity[u8((b + 1) & 7)] == 0) f ^= FP;
        f = u8((f & ~FH) | ((b & 0x0F) == 0x0F ? FH : 0));
      }
    } else if (kParity[u8(b & 7)] == 0) {
      f ^= FP;
    }
  }
  setF(f);
}
