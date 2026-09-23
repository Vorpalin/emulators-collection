#pragma once

#include <cstdint>
#include <string>

#include "Bus.hh"
#include "CPU65.hh"
#include "Cartbridge.hh"
#include "MOS6532.hh"
#include "TIA1A.hh"

class Atari2600Bus : public Bus {
 public:
  Atari2600Bus();

  void loadROM(std::string& filename) override;
  void reset() override;

  uint8_t readMemory(uint16_t address) override;
  void writeMemory(uint16_t address, uint8_t value) override;

  void tick() override;  // Advance the bus by one clock cycle

  const uint8_t* frame() const { return tia1a.frame(); }
  bool frameReady() const { return tia1a.frameReady(); }
  void clearFrameReady() { tia1a.clearFrameReady(); }

  void setInput(uint8_t swcha, uint8_t swchb, bool fire0, bool fire1);

 private:
  CPU65 cpu;              // Instance of the CPU65 class for the Atari 2600
  Cartbridge cartbridge;  // Instance of the Cartbridge class for ROM handling
  MOS6532 mos6532;  // Instance of the MOS6532 class for RAM and I/O handling
  TIA1A tia1a;      // Instance of the TIA1A class for video output
};
