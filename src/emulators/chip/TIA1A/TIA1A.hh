#pragma once
#include <array>
#include <cstdint>

class TIA1A {
 public:
  static constexpr int kWidth = 160;
  static constexpr int kHeight = 312;
  static constexpr int kLineClocks = 228;
  static constexpr int kHBlank = 68;

  TIA1A() { reset(); }
  void reset();

  void tick();

  void write(uint16_t addr, uint8_t value);
  uint8_t read(uint16_t addr) const;

  bool cpuHalted() const;

  bool frameReady() const;
  void clearFrameReady();

  const uint8_t* frame() const;

  void setFire(int player, bool pressed);

 private:
  struct Player {
    uint8_t grp = 0, grpOld = 0, nusiz = 0;
    bool reflect = false, vdel = false;
    int pos = 0;
    int8_t hm = 0;
  };
  struct Missile {
    bool enabled = false, resetToPlayer = false;
    int pos = 0;
    int8_t hm = 0;
  };
  struct Ball {
    bool enabled = false, enabledOld = false, vdel = false;
    int pos = 0;
    int8_t hm = 0;
  };

  enum Cx {
    M0P1,
    M0P0,
    M1P0,
    M1P1,
    P0PF,
    P0BL,
    P1PF,
    P1BL,
    M0PF,
    M0BL,
    M1PF,
    M1BL,
    BLPF,
    P0P1,
    M0M1
  };

  uint8_t renderPixel(int x);
  bool playfieldBit(int x) const;
  bool playerPixel(const Player& p, int x) const;
  bool missilePixel(const Missile& m, const Player& p, int x) const;
  bool ballPixel(int x) const;
  void applyHmove();

  Player p_[2];
  Missile m_[2];
  Ball bl_;

  uint8_t pf0_ = 0, pf1_ = 0, pf2_ = 0, ctrlpf_ = 0;
  uint8_t colup_[2] = {0, 0}, colupf_ = 0, colubk_ = 0;
  uint16_t cx_ = 0;

  bool vsync_ = false, vblank_ = false, wsync_ = false;
  bool hmoveBlank_ = false, frameReady_ = false;
  bool fire_[2] = {false, false};

  int hpos_ = 0, vpos_ = 0;
  std::array<uint8_t, kWidth * kHeight> frame_{};
};
