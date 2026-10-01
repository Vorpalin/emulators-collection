#include "emulators/audio/APU/APU.hh"

static const uint8_t DUTY_CYCLES[4][8] = {
    {0, 0, 0, 0, 0, 0, 0, 1},  // 12.5%
    {1, 0, 0, 0, 0, 0, 0, 1},  // 25%
    {1, 0, 0, 0, 0, 1, 1, 1},  // 50%
    {0, 1, 1, 1, 1, 1, 1, 0}   // -25%
};

APU::APU() { reset(); }

APU::~APU() {}

void APU::reset() {
  audioEnabled = false;
  frameStep = 0;
  frameTimer = 0;

  channel1 = {};
  channel2 = {};
  channel3 = {};
  channel4 = {};

  nr50 = 0;
  nr51 = 0;
}

void APU::tick(uint32_t cycles) {
  if (!audioEnabled) {
    return;
  }

  frameTimer += cycles;
  if (frameTimer >= 8192) {
    frameTimer -= 8192;
    stepFrame();
  }

  channel1.timer -= cycles;
  while (channel1.timer <= 0) {
    int raw = channel1.nr13 | ((channel1.nr14 & 0x07) << 8);
    channel1.timer += (2048 - raw) * 4;
    channel1.duty = (channel1.duty + 1) % 8;
  }

  channel2.timer -= cycles;
  while (channel2.timer <= 0) {
    int raw = channel2.nr23 | ((channel2.nr24 & 0x07) << 8);
    channel2.timer += (2048 - raw) * 4;
    channel2.duty = (channel2.duty + 1) % 8;
  }

  channel3.timer -= cycles;
  while (channel3.timer <= 0) {
    int raw = channel3.nr33 | ((channel3.nr34 & 0x07) << 8);
    channel3.timer += (2048 - raw) * 2;
    channel3.position = (channel3.position + 1) % 32;
  }

  channel4.timer -= cycles;
  while (channel4.timer <= 0) {
    const int divisors[8] = {8, 16, 32, 48, 64, 80, 96, 112};
    int divisor = divisors[channel4.nr43 & 0x07];
    int shift = (channel4.nr43 >> 4) & 0x07;

    int currentPeriod = divisor << shift;

    channel4.timer += currentPeriod;

    int xorBit = ((channel4.lfsr & 0x01) ^ ((channel4.lfsr >> 1) & 0x01));
    channel4.lfsr >>= 1;
    channel4.lfsr |= (xorBit << 14);

    if (channel4.nr43 & 0x08) {
      channel4.lfsr = (channel4.lfsr & ~0x40) | (xorBit << 6);
    }
  }
}

void APU::stepFrame() {
  frameStep = (frameStep + 1) % 8;

  if (frameStep % 2 == 0) {
    stepLength();
  }

  if (frameStep == 7) {
    stepVolumeEnvelope();
  }

  if (frameStep == 2 || frameStep == 6) {
    stepSweep();
  }
}

void APU::stepLength() {
  if (channel1.nr14 & 0x40 && channel1.length > 0) {
    channel1.length--;
    if (channel1.length == 0) {
      channel1.enabled = false;
    }
  }

  if (channel2.nr24 & 0x40 && channel2.length > 0) {
    channel2.length--;
    if (channel2.length == 0) {
      channel2.enabled = false;
    }
  }

  if (channel3.nr34 & 0x40 && channel3.length > 0) {
    channel3.length--;
    if (channel3.length == 0) {
      channel3.enabled = false;
    }
  }

  if (channel4.nr44 & 0x40 && channel4.length > 0) {
    channel4.length--;
    if (channel4.length == 0) {
      channel4.enabled = false;
    }
  }
}

void APU::stepVolumeEnvelope() {
  auto updateEnvelope = [](int& volume, int& envelopeTimer, uint8_t nr,
                           bool& enabled) {
    if (!enabled) return;

    int period = nr & 0x07;
    if (period == 0) return;

    envelopeTimer--;
    if (envelopeTimer <= 0) {
      envelopeTimer = period;
      bool direction = (nr & 0x08);

      if (direction && volume < 15) {
        volume++;
      } else if (!direction && volume > 0) {
        volume--;
      }
    }
  };

  updateEnvelope(channel1.volume, channel1.envelopeTimer, channel1.nr12,
                 channel1.enabled);
  updateEnvelope(channel2.volume, channel2.envelopeTimer, channel2.nr22,
                 channel2.enabled);
  updateEnvelope(channel4.volume, channel4.envelopeTimer, channel4.nr42,
                 channel4.enabled);
}

void APU::stepSweep() {
  if (!channel1.sweepEnabled) return;

  channel1.sweepTimer--;
  if (channel1.sweepTimer <= 0) {
    int period = (channel1.nr10 >> 4) & 0x07;
    if (period == 0) {
      period = 8;
    }
    int shift = channel1.nr10 & 0x07;
    channel1.sweepTimer = period;

    if (period > 0 && shift > 0) {
      int newFrequency = calculateSweepFrequency();
      if (newFrequency <= 2047) {
        channel1.shadowFrequency = newFrequency;
        channel1.nr13 = newFrequency & 0xFF;
        channel1.nr14 = (channel1.nr14 & 0xF8) | ((newFrequency >> 8) & 0x07);
        calculateSweepFrequency();  // Check for overflow
      }
    }
  }
}

int APU::calculateSweepFrequency() {
  int newFrequency = channel1.shadowFrequency >> (channel1.nr10 & 0x07);
  if (channel1.nr10 & 0x08) {
    newFrequency = channel1.shadowFrequency - newFrequency;
  } else {
    newFrequency = channel1.shadowFrequency + newFrequency;
  }

  if (newFrequency > 2047) {
    channel1.enabled = false;
  }
  return newFrequency;
}

void APU::triggerChannel1() {
  channel1.enabled = (channel1.nr12 & 0xF8) != 0;  // DAC off -> stays silent
  channel1.timer = (2048 - (channel1.nr13 | ((channel1.nr14 & 0x07) << 8))) * 4;
  if (channel1.length == 0) {
    channel1.length = 64;
  }
  channel1.envelopeTimer = channel1.nr12 & 0x07;
  channel1.volume = (channel1.nr12 >> 4);

  channel1.shadowFrequency = (channel1.nr13 | (channel1.nr14 & 0x07) << 8);

  int sweepPeriod = (channel1.nr10 >> 4) & 0x07;
  int sweepShift = channel1.nr10 & 0x07;

  channel1.sweepTimer = (sweepPeriod == 0) ? 8 : sweepPeriod;
  channel1.sweepEnabled = (sweepPeriod > 0 || sweepShift > 0);
  if (sweepShift > 0) {
    calculateSweepFrequency();
  }
}

void APU::triggerChannel2() {
  channel2.enabled = (channel2.nr22 & 0xF8) != 0;
  channel2.timer = (2048 - (channel2.nr23 | ((channel2.nr24 & 0x07) << 8))) * 4;
  if (channel2.length == 0) {
    channel2.length = 64;
  }
  channel2.envelopeTimer = channel2.nr22 & 0x07;
  channel2.volume = (channel2.nr22 >> 4);
}

void APU::triggerChannel3() {
  channel3.enabled = (channel3.nr30 & 0x80) != 0;  // NR30 bit 7 = DAC power
  channel3.timer = (2048 - (channel3.nr33 | ((channel3.nr34 & 0x07) << 8))) * 2;
  if (channel3.length == 0) {
    channel3.length = 256;
  }
  channel3.position = 0;
}

void APU::triggerChannel4() {
  channel4.enabled = (channel4.nr42 & 0xF8) != 0;
  if (channel4.length == 0) {
    channel4.length = 64;
  }
  channel4.envelopeTimer = channel4.nr42 & 0x07;
  channel4.volume = (channel4.nr42 >> 4);
  channel4.lfsr = 0x7FFF;  // Reset LFSR
}

uint8_t APU::read(uint16_t addr) {
  if (addr >= 0xFF30 && addr <= 0xFF3F) {
    return channel3.waveTable[addr - 0xFF30];
  }
  switch (addr) {
    case 0xFF10:
      return channel1.nr10 | 0x80;  // Bit 7 is always set
    case 0xFF11:
      return channel1.nr11 | 0x3F;  // Bits 6-7 are unused
    case 0xFF12:
      return channel1.nr12;
    case 0xFF14:
      return channel1.nr14 | 0xBF;  // Bit 6 is unused

    case 0xFF16:
      return channel2.nr21 | 0x3F;  // Bits 6-7 are unused
    case 0xFF17:
      return channel2.nr22;
    case 0xFF19:
      return channel2.nr24 | 0xBF;  // Bit 6 is unused

    case 0xFF1A:
      return channel3.nr30 | 0x7F;  // Bit 7 is unused
    case 0xFF1C:
      return channel3.nr32 | 0x9F;  // Bits 5-6 are unused
    case 0xFF1E:
      return channel3.nr34 | 0xBF;  // Bit 6 is unused

    case 0xFF20:
      return channel4.nr41 | 0xFF;
    case 0xFF21:
      return channel4.nr42;
    case 0xFF22:
      return channel4.nr43;
    case 0xFF23:
      return channel4.nr44 | 0xBF;  // Bit 6 is unused

    case 0xFF24:
      return nr50;
    case 0xFF25:
      return nr51;
    case 0xFF26:
      return (audioEnabled ? 0x80 : 0x00) | (channel4.enabled ? 0x08 : 0x00) |
             (channel3.enabled ? 0x04 : 0x00) |
             (channel2.enabled ? 0x02 : 0x00) |
             (channel1.enabled ? 0x01 : 0x00);

    default:
      return 0xFF;  // Unhandled addresses return 0xFF
  }
}

void APU::write(uint16_t addr, uint8_t data) {
  if (addr >= 0xFF30 && addr <= 0xFF3F) {
    channel3.waveTable[addr - 0xFF30] = data;
    return;
  }
  switch (addr) {
    case 0xFF10:
      channel1.nr10 = data;
      break;
    case 0xFF11:
      channel1.nr11 = data;
      channel1.length = 64 - (data & 0x3F);
      break;
    case 0xFF12:
      channel1.nr12 = data;
      break;
    case 0xFF13:
      channel1.nr13 = data;
      break;
    case 0xFF14:
      channel1.nr14 = data;
      if (data & 0x80) {  // Trigger
        triggerChannel1();
      }
      break;

    case 0xFF16:
      channel2.nr21 = data;
      channel2.length = 64 - (data & 0x3F);
      break;
    case 0xFF17:
      channel2.nr22 = data;
      break;
    case 0xFF18:
      channel2.nr23 = data;
      break;
    case 0xFF19:
      channel2.nr24 = data;
      if (data & 0x80) {  // Trigger
        triggerChannel2();
      }
      break;

    case 0xFF1A:
      channel3.nr30 = data;
      if (!(data & 0x80)) channel3.enabled = false;  // DAC off
      break;
    case 0xFF1B:
      channel3.nr31 = data;
      channel3.length = 256 - data;
      break;
    case 0xFF1C:
      channel3.nr32 = data;
      break;
    case 0xFF1D:
      channel3.nr33 = data;
      break;
    case 0xFF1E:
      channel3.nr34 = data;
      if (data & 0x80) {  // Trigger
        triggerChannel3();
      }
      break;

    case 0xFF20:
      channel4.nr41 = data;
      channel4.length = 64 - (data & 0x3F);
      break;
    case 0xFF21:
      channel4.nr42 = data;
      break;
    case 0xFF22:
      channel4.nr43 = data;
      break;
    case 0xFF23:
      channel4.nr44 = data;
      if (data & 0x80) {  // Trigger
        triggerChannel4();
      }
      break;

    case 0xFF24:
      nr50 = data;
      break;
    case 0xFF25:
      nr51 = data;
      break;

    case 0xFF26:
      audioEnabled = (data & 0x80);
      if (!audioEnabled) {
        nr50 = 0x00;
        nr51 = 0x00;
      }
      break;
  }
}

void APU::getStereoSample(float& left, float& right) {
  left = 0.0f;
  right = 0.0f;
  if (!audioEnabled) {
    return;
  }

  float output1 = 0.0f;
  float output2 = 0.0f;
  float output3 = 0.0f;
  float output4 = 0.0f;

  if (channel1.enabled) {
    int duty = (channel1.nr11 >> 6) & 0x03;
    int dutyValue = DUTY_CYCLES[duty][channel1.duty];
    if (!dutyValue) {
      dutyValue = -1;  // Invert the duty value for silence
    }
    output1 = static_cast<float>(dutyValue) *
              static_cast<float>(channel1.volume) / 15.0f;
  }

  if (channel2.enabled) {
    int duty = (channel2.nr21 >> 6) & 0x03;
    int dutyValue = DUTY_CYCLES[duty][channel2.duty];
    if (!dutyValue) {
      dutyValue = -1;  // Invert the duty value for silence
    }
    output2 = static_cast<float>(dutyValue) *
              static_cast<float>(channel2.volume) / 15.0f;
  }

  if (channel3.enabled) {
    int sampleByte = channel3.waveTable[channel3.position / 2];
    int sampleValue =
        (channel3.position % 2 == 0) ? (sampleByte >> 4) : (sampleByte & 0x0F);

    int volumeCode = (channel3.nr32 >> 5) & 0x03;
    if (volumeCode == 2) {
      sampleValue >>= 1;  // 50%
    } else if (volumeCode == 3) {
      sampleValue >>= 2;  // 25%
    }

    if (volumeCode != 0) {  // 0 = mute: output silence, not a -1.0 DC level
      output3 = (static_cast<float>(sampleValue) / 7.5f) - 1.0f;  // [-1, 1]
    }
  }

  if (channel4.enabled) {
    int lfsrBit = channel4.lfsr & 0x01;
    output4 = (lfsrBit == 0) ? 1.0f : -1.0f;  // Invert the LFSR output
    output4 *= static_cast<float>(channel4.volume) / 15.0f;
  }

  float leftOutput = 0.0f;
  float rightOutput = 0.0f;

  if (nr51 & 0x10) leftOutput += output1;
  if (nr51 & 0x20) leftOutput += output2;
  if (nr51 & 0x40) leftOutput += output3;
  if (nr51 & 0x80) leftOutput += output4;

  if (nr51 & 0x01) rightOutput += output1;
  if (nr51 & 0x02) rightOutput += output2;
  if (nr51 & 0x04) rightOutput += output3;
  if (nr51 & 0x08) rightOutput += output4;

  // NR50: bits 6-4 = left master volume, bits 2-0 = right (0 still means 1/8).
  float leftVolume = static_cast<float>(((nr50 >> 4) & 0x07) + 1) / 8.0f;
  float rightVolume = static_cast<float>((nr50 & 0x07) + 1) / 8.0f;
  left = leftOutput * leftVolume / 4.0f;
  right = rightOutput * rightVolume / 4.0f;
}

float APU::getSample() {
  float l, r;
  getStereoSample(l, r);
  return (l + r) * 0.5f;
}
