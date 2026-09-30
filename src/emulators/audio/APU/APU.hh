#pragma once

#include <array>
#include <cstdint>
#include <memory>

#include "emulators/bus/Bus.hh"

struct Channel1 {
  bool enabled = false;

  uint8_t nr10 = 0;
  uint8_t nr11 = 0;
  uint8_t nr12 = 0;
  uint8_t nr13 = 0;
  uint8_t nr14 = 0;

  int timer = 0;
  int duty = 0;

  int length = 0;
  int volume = 0;
  int envelopeTimer = 0;

  int sweepTimer = 0;
  int shadowFrequency = 0;
  bool sweepEnabled = false;

  uint8_t output = 0;
};

struct Channel2 {
  bool enabled = false;

  uint8_t nr21 = 0;
  uint8_t nr22 = 0;
  uint8_t nr23 = 0;
  uint8_t nr24 = 0;

  int timer = 0;
  int duty = 0;

  int length = 0;
  int volume = 0;
  int envelopeTimer = 0;

  uint8_t output = 0;
};

struct Channel3 {
  bool enabled = false;

  uint8_t nr30 = 0;
  uint8_t nr31 = 0;
  uint8_t nr32 = 0;
  uint8_t nr33 = 0;
  uint8_t nr34 = 0;

  int timer = 0;
  int length = 0;
  int position = 0;

  std::array<uint8_t, 16> waveTable{};

  uint8_t output = 0;
};

struct Channel4 {
  bool enabled = false;

  uint8_t nr41 = 0;
  uint8_t nr42 = 0;
  uint8_t nr43 = 0;
  uint8_t nr44 = 0;

  int timer = 0;
  int length = 0;
  int volume = 0;
  int envelopeTimer = 0;

  uint16_t lfsr = 0x7FFF;

  uint8_t output = 0;
};

class APU {
 public:
  APU();
  ~APU();

  void connectBus(std::shared_ptr<Bus> bus);

  void tick(uint8_t cycles);

  void reset();
  void step(uint32_t cycles);
  void write(uint16_t addr, uint8_t data);
  uint8_t read(uint16_t addr);

  float getSample();

 private:
  std::shared_ptr<Bus> bus;
  bool audioEnabled = false;

  int frameStep = 0;
  int frameTimer = 0;

  void stepFrame();
  void stepLength();
  void stepVolumeEnvelope();
  void stepSweep();

  struct Channel1 channel1;
  struct Channel2 channel2;
  struct Channel3 channel3;
  struct Channel4 channel4;

  uint8_t nr50 = 0;
  uint8_t nr51 = 0;

  uint8_t getDutyOutput(int duty, int step);
  int calculateSweepFrequency();
  void triggerChannel1();
  void triggerChannel2();
  void triggerChannel3();
  void triggerChannel4();
};
