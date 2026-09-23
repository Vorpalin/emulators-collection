#include "TIA1A.hh"
#include <cstring>

namespace
{

  struct CopyInfo
  {
    int count;
    int offset[3];
    int scale;
  };


  constexpr CopyInfo kCopies[8] = {
    {1, {0, 0, 0}, 1},
    {2, {0, 16, 0}, 1},
    {2, {0, 32, 0}, 1},
    {3, {0, 16, 32}, 1},
    {2, {0, 64, 0}, 1},
    {1, {0, 0, 0}, 2},
    {3, {0, 32, 64}, 1},
    {1, {0, 0, 0}, 4},
  };

  inline int mod160(int v)
  {
    v %= 160;
    return v < 0 ? v + 160 : v;
  }

}

void TIA1A::reset()
{
  p_[0] = p_[1] = Player{};
  m_[0] = m_[1] = Missile{};
  bl_ = Ball{};
  pf0_ = pf1_ = pf2_ = ctrlpf_ = 0;
  colup_[0] = colup_[1] = colupf_ = colubk_ = 0;
  cx_ = 0;
  vsync_ = vblank_ = wsync_ = hmoveBlank_ = frameReady_ = false;
  hpos_ = vpos_ = 0;
  frame_.fill(0);
}

void TIA1A::tick()
{
  const int x = hpos_ - kHBlank;
  if (x >= 0 && vpos_ < kHeight)
    {
      uint8_t c = renderPixel(x);
      if (hmoveBlank_ && x < 8)
        c = 0;
      frame_[vpos_ * kWidth + x] = c;
    }
  if (++hpos_ == kLineClocks)
    {
      hpos_ = 0;
      wsync_ = false;
      hmoveBlank_ = false;
      if (++vpos_ >= kHeight)
        vpos_ = 0;
    }
}

bool TIA1A::playfieldBit(int x) const
{
  int i = x / 4;
  if (i >= 20)
    i = (ctrlpf_ & 1) ? 39 - i : i - 20;
  if (i < 4)
    return (pf0_ >> (4 + i)) & 1;
  if (i < 12)
    return (pf1_ >> (7 - (i - 4))) & 1;
  return (pf2_ >> (i - 12)) & 1;
}

bool TIA1A::playerPixel(const Player& p, int x) const
{
  const CopyInfo& ci = kCopies[p.nusiz & 7];
  const int d = mod160(x - p.pos);
  const uint8_t g = p.vdel ? p.grpOld : p.grp;
  for (int i = 0; i < ci.count; ++i)
    {
      int o = d - ci.offset[i];
      if (o >= 0 && o < 8 * ci.scale)
        {
          int bit = o / ci.scale;
          return (g >> (p.reflect ? bit : 7 - bit)) & 1;
        }
    }
  return false;
}

bool TIA1A::missilePixel(const Missile& m, const Player& p, int x) const
{
  if (!m.enabled || m.resetToPlayer)
    return false;
  const CopyInfo& ci = kCopies[p.nusiz & 7];
  const int width = 1 << ((p.nusiz >> 4) & 3);
  const int d = mod160(x - m.pos);
  for (int i = 0; i < ci.count; ++i)
    {
      int o = d - ci.offset[i];
      if (o >= 0 && o < width)
        return true;
    }
  return false;
}

bool TIA1A::ballPixel(int x) const
{
  const bool en = bl_.vdel ? bl_.enabledOld : bl_.enabled;
  if (!en)
    return false;
  const int width = 1 << ((ctrlpf_ >> 4) & 3);
  return mod160(x - bl_.pos) < width;
}

uint8_t TIA1A::renderPixel(int x)
{
  const bool p0 = playerPixel(p_[0], x);
  const bool p1 = playerPixel(p_[1], x);
  const bool m0 = missilePixel(m_[0], p_[0], x);
  const bool m1 = missilePixel(m_[1], p_[1], x);
  const bool bl = ballPixel(x);
  const bool pf = playfieldBit(x);

  auto set = [&](bool c, int bit) {
    if (c)
      cx_ |= 1u << bit;
  };
  set(m0 && p1, M0P1);
  set(m0 && p0, M0P0);
  set(m1 && p0, M1P0);
  set(m1 && p1, M1P1);
  set(p0 && pf, P0PF);
  set(p0 && bl, P0BL);
  set(p1 && pf, P1PF);
  set(p1 && bl, P1BL);
  set(m0 && pf, M0PF);
  set(m0 && bl, M0BL);
  set(m1 && pf, M1PF);
  set(m1 && bl, M1BL);
  set(bl && pf, BLPF);
  set(p0 && p1, P0P1);
  set(m0 && m1, M0M1);

  if (vblank_)
    return 0;

  const bool priority = ctrlpf_ & 0x04;
  const bool score = (ctrlpf_ & 0x02) && !priority;
  const uint8_t pfColor = score ? colup_[x >= 80 ? 1 : 0] : colupf_;

  if (priority)
    {
      if (pf || bl)
        return pfColor;
      if (p0 || m0)
        return colup_[0];
      if (p1 || m1)
        return colup_[1];
    }
  else
    {
      if (p0 || m0)
        return colup_[0];
      if (p1 || m1)
        return colup_[1];
      if (pf || bl)
        return pfColor;
    }
  return colubk_;
}

void TIA1A::applyHmove()
{
  for (auto& p : p_)
    p.pos = mod160(p.pos - p.hm);
  for (auto& m : m_)
    m.pos = mod160(m.pos - m.hm);
  bl_.pos = mod160(bl_.pos - bl_.hm);

  if (hpos_ < kHBlank)
    hmoveBlank_ = true;
}

void TIA1A::write(uint16_t addr, uint8_t v)
{
  addr &= 0x3F;
  const int x = hpos_ - kHBlank;

  auto resPlayer = [&] { return x < 0 ? 3 : mod160(x + 5); };
  auto resMissile = [&] { return x < 0 ? 2 : mod160(x + 4); };
  auto hmValue = [&](uint8_t val) {
    return (int8_t)((((val >> 4) & 0x0F) ^ 8) - 8);
  };

  switch (addr)
    {
    case 0x00:
      {
        bool on = v & 0x02;
        if (vsync_ && !on)
          {
            vpos_ = 0;
            frameReady_ = true;
          }
        vsync_ = on;
        break;
      }
    case 0x01:
      vblank_ = v & 0x02;
      break;
    case 0x02:
      wsync_ = true;
      break;
    case 0x03:
      hpos_ = kLineClocks - 3;
      break;
    case 0x04:
      p_[0].nusiz = v;
      break;
    case 0x05:
      p_[1].nusiz = v;
      break;
    case 0x06:
      colup_[0] = v & 0xFE;
      break;
    case 0x07:
      colup_[1] = v & 0xFE;
      break;
    case 0x08:
      colupf_ = v & 0xFE;
      break;
    case 0x09:
      colubk_ = v & 0xFE;
      break;
    case 0x0A:
      ctrlpf_ = v;
      break;
    case 0x0B:
      p_[0].reflect = v & 0x08;
      break;
    case 0x0C:
      p_[1].reflect = v & 0x08;
      break;
    case 0x0D:
      pf0_ = v;
      break;
    case 0x0E:
      pf1_ = v;
      break;
    case 0x0F:
      pf2_ = v;
      break;
    case 0x10:
      p_[0].pos = resPlayer();
      break;
    case 0x11:
      p_[1].pos = resPlayer();
      break;
    case 0x12:
      m_[0].pos = resMissile();
      break;
    case 0x13:
      m_[1].pos = resMissile();
      break;
    case 0x14:
      bl_.pos = resMissile();
      break;
    case 0x15:
    case 0x16:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
      break; // TODO : audio
    case 0x1B:
      p_[0].grp = v;
      p_[1].grpOld = p_[1].grp;
      break;
    case 0x1C:
      p_[1].grp = v;
      p_[0].grpOld = p_[0].grp;
      bl_.enabledOld = bl_.enabled;
      break;
    case 0x1D:
      m_[0].enabled = v & 0x02;
      break;
    case 0x1E:
      m_[1].enabled = v & 0x02;
      break;
    case 0x1F:
      bl_.enabled = v & 0x02;
      break;
    case 0x20:
      p_[0].hm = hmValue(v);
      break;
    case 0x21:
      p_[1].hm = hmValue(v);
      break;
    case 0x22:
      m_[0].hm = hmValue(v);
      break;
    case 0x23:
      m_[1].hm = hmValue(v);
      break;
    case 0x24:
      bl_.hm = hmValue(v);
      break;
    case 0x25:
      p_[0].vdel = v & 1;
      break;
    case 0x26:
      p_[1].vdel = v & 1;
      break;
    case 0x27:
      bl_.vdel = v & 1;
      break;
    case 0x28:
    case 0x29:
      {
        int i = addr - 0x28;
        bool on = v & 0x02;
        if (m_[i].resetToPlayer && !on)
          {
            int n = p_[i].nusiz & 7;
            m_[i].pos = mod160(p_[i].pos + (n == 5 ? 6 : n == 7 ? 10 : 3));
          }
        m_[i].resetToPlayer = on;
        break;
      }
    case 0x2A:
      applyHmove();
      break;
    case 0x2B:
      p_[0].hm = p_[1].hm = m_[0].hm = m_[1].hm = bl_.hm = 0;
      break;
    case 0x2C:
      cx_ = 0;
      break;
    default:
      break;
    }
}

uint8_t TIA1A::read(uint16_t addr) const
{
  auto b = [&](int bit) -> uint8_t { return (cx_ >> bit) & 1; };
  switch (addr & 0x0F)
    {
    case 0x00:
      return b(M0P1) << 7 | b(M0P0) << 6;
    case 0x01:
      return b(M1P0) << 7 | b(M1P1) << 6;
    case 0x02:
      return b(P0PF) << 7 | b(P0BL) << 6;
    case 0x03:
      return b(P1PF) << 7 | b(P1BL) << 6;
    case 0x04:
      return b(M0PF) << 7 | b(M0BL) << 6;
    case 0x05:
      return b(M1PF) << 7 | b(M1BL) << 6;
    case 0x06:
      return b(BLPF) << 7;
    case 0x07:
      return b(P0P1) << 7 | b(M0M1) << 6;
    case 0x0C:
      return fire_[0] ? 0x00 : 0x80;
    case 0x0D:
      return fire_[1] ? 0x00 : 0x80;
    default:
      return 0x00;
    }
}

void TIA1A::setFire(int player, bool pressed) { 
  fire_[player & 1] = pressed; 
}

bool TIA1A::cpuHalted() const 
{ 
  return wsync_; 
}

bool TIA1A::frameReady() const 
{
    return frameReady_; 
}

void TIA1A::clearFrameReady() 
{ 
  frameReady_ = false; 
}

const uint8_t* TIA1A::frame() const 
{ 
  return frame_.data(); 
}