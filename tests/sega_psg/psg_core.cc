// Sega PSG (SN76489) checks: register protocol, tone and noise timing,
// volume curve, sample-playback behaviour, Game Gear stereo and sample rate.
//
// Most tests use a sample rate equal to the chip tick rate (223,721 Hz) with
// the high-pass filter off, so one output sample is one chip tick.

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

#include "emulators/audio/sega_psg/SegaPSG.hh"

namespace {

int failures = 0;

#define CHECK(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);      \
      failures++;                                                      \
    }                                                                  \
  } while (0)

// 3,579,545 / 16 = 223,721.56 Hz, rounded up so every tick yields a sample.
constexpr int kTickRate = 223722;

bool near(float a, float b) { return std::fabs(a - b) < 1e-4f; }

// Chip with one sample per tick and no filtering.
void exactRig(SegaPSG &psg) {
  psg.setSampleRate(kTickRate);
  psg.setHighPass(false);
}

void ticks(SegaPSG &psg, int n) { psg.step(static_cast<uint32_t>(n * SegaPSG::kCyclesPerTick)); }

// Writes a 10-bit tone period using the two-byte protocol.
void setTone(SegaPSG &psg, int ch, int period) {
  psg.write(static_cast<uint8_t>(0x80 | (ch << 5) | (period & 0x0F)));
  psg.write(static_cast<uint8_t>((period >> 4) & 0x3F));
}

void setVolume(SegaPSG &psg, int ch, int att) {
  psg.write(static_cast<uint8_t>(0x90 | (ch << 5) | (att & 0x0F)));
}

void testRegisterProtocol() {
  SegaPSG psg;
  CHECK(psg.getState().volume[0] == 15 && psg.getState().volume[3] == 15);

  psg.write(0x83);  // Latch tone 0, low nibble 3.
  CHECK(psg.getState().tonePeriod[0] == 0x003);
  psg.write(0x12);  // Data byte: high 6 bits.
  CHECK(psg.getState().tonePeriod[0] == 0x123);
  psg.write(0x8F);  // Re-latch tone 0, new low nibble keeps the high bits.
  CHECK(psg.getState().tonePeriod[0] == 0x12F);

  setTone(psg, 1, 0x3FF);
  CHECK(psg.getState().tonePeriod[1] == 0x3FF);
  setTone(psg, 2, 0x155);
  CHECK(psg.getState().tonePeriod[2] == 0x155);
  CHECK(psg.getState().tonePeriod[0] == 0x12F);

  psg.write(0x93);  // Tone 0 attenuation = 3.
  CHECK(psg.getState().volume[0] == 3);
  psg.write(0x05);  // Data byte after a volume latch: low 4 bits.
  CHECK(psg.getState().volume[0] == 5);
  setVolume(psg, 3, 7);
  CHECK(psg.getState().volume[3] == 7);

  psg.write(0xE5);  // Noise control: white noise (bit 2), rate 1.
  CHECK(psg.getState().noiseControl == 5);
  CHECK(psg.getState().lfsr == 0x8000);
  psg.write(0x03);  // Data byte after the noise latch.
  CHECK(psg.getState().noiseControl == 3);
}

void testSquareWave() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 0, 4);
  setVolume(psg, 0, 0);
  ticks(psg, 64);
  const auto &s = psg.samples();
  CHECK(psg.frameCount() == 64);

  // Left and right carry the same signal; skip the first samples (start-up).
  const float amp = 0.25f;
  int transitions = 0;
  int run = 0;
  bool high = s[2 * 8] > 0.1f;
  bool firstEdgeSeen = false;  // The first run started before sample 8.
  for (size_t i = 8; i < 64; i++) {
    const float l = s[2 * i], r = s[2 * i + 1];
    CHECK(near(l, r));
    CHECK(near(l, 0.0f) || near(l, amp));
    const bool h = l > 0.1f;
    if (h != high) {
      if (firstEdgeSeen) CHECK(run == 4);  // Toggles every `period` ticks.
      firstEdgeSeen = true;
      transitions++;
      run = 0;
      high = h;
    }
    run++;
  }
  CHECK(transitions >= 10);
}

void testPeriodTwoIsFastestSquare() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 0, 2);
  setVolume(psg, 0, 0);
  ticks(psg, 40);
  int changes = 0;
  const auto &s = psg.samples();
  for (size_t i = 11; i < 40; i++)
    if ((s[2 * i] > 0.1f) != (s[2 * (i - 1)] > 0.1f)) changes++;
  CHECK(changes >= 12);  // Toggles every 2 ticks.
}

void testSamplePlaybackPeriodZero() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 0, 0);   // Period 0: output stays high.
  setVolume(psg, 0, 0);
  ticks(psg, 16);
  for (size_t i = 0; i < 16; i++) CHECK(near(psg.samples()[2 * i], 0.25f));

  // Changing the volume directly modulates the output (PCM playback).
  psg.clearSamples();
  setVolume(psg, 0, 15);
  ticks(psg, 4);
  for (size_t i = 0; i < 4; i++) CHECK(near(psg.samples()[2 * i], 0.0f));
}

void testVolumeCurve() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 0, 0);  // Constant high: output = channel amplitude.
  float prev = 1.0f;
  for (int att = 0; att < 15; att++) {
    setVolume(psg, 0, att);
    psg.clearSamples();
    ticks(psg, 2);
    const float a = psg.samples()[2];
    const float expected = static_cast<float>(0.25 * std::pow(10.0, -0.1 * att));
    CHECK(near(a, expected));  // 2 dB per step.
    CHECK(a < prev);
    prev = a;
  }
  setVolume(psg, 0, 15);
  psg.clearSamples();
  ticks(psg, 2);
  CHECK(near(psg.samples()[2], 0.0f));
}

void testChannelsMix() {
  SegaPSG psg;
  exactRig(psg);
  for (int ch = 0; ch < 3; ch++) {
    setTone(psg, ch, 0);
    setVolume(psg, ch, 0);
  }
  ticks(psg, 4);
  CHECK(near(psg.samples()[6], 0.75f));  // Three full-volume channels.
}

void testPeriodicNoise() {
  SegaPSG psg;
  exactRig(psg);
  psg.write(0xE0);  // Periodic noise, fastest rate (reload 0x10).
  CHECK(psg.getState().lfsr == 0x8000);
  // Rising edges of the noise flip-flop fall on ticks 1, 33, 65, ...
  ticks(psg, 449);  // 15 shifts: the single set bit has travelled to bit 0.
  CHECK(psg.getState().lfsr == 0x0001);
  ticks(psg, 32);   // The 16th shift wraps it around.
  CHECK(psg.getState().lfsr == 0x8000);
}

void testNoiseFollowsTone2() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 2, 3);
  psg.write(0xE3);  // Periodic noise clocked by tone 2.
  ticks(psg, 19);   // Rising edges at ticks 1, 7, 13, 19: four shifts.
  CHECK(psg.getState().lfsr == 0x0800);
}

void testWhiteNoiseFeedback() {
  SegaPSG psg;
  exactRig(psg);
  psg.write(0xE4);  // White noise, fastest rate.
  setVolume(psg, 3, 0);
  ticks(psg, 2000);
  // The register must keep changing and never lock up at zero.
  uint16_t first = psg.getState().lfsr;
  CHECK(first != 0);
  bool varied = false;
  for (int i = 0; i < 200; i++) {
    ticks(psg, 32);
    uint16_t v = psg.getState().lfsr;
    CHECK(v != 0);
    if (v != first) varied = true;
  }
  CHECK(varied);
  // Noise at full volume produces both silent and loud samples.
  psg.clearSamples();
  ticks(psg, 4000);
  bool low = false, high = false;
  for (size_t i = 0; i < psg.frameCount(); i++) {
    if (near(psg.samples()[2 * i], 0.0f)) low = true;
    if (near(psg.samples()[2 * i], 0.25f)) high = true;
  }
  CHECK(low && high);
}

void testGameGearStereo() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 0, 0);
  setVolume(psg, 0, 0);
  psg.writeStereo(0x01);  // Tone 0 on the right only.
  ticks(psg, 4);
  CHECK(near(psg.samples()[0], 0.0f) && near(psg.samples()[1], 0.25f));
  psg.clearSamples();
  psg.writeStereo(0x10);  // Tone 0 on the left only.
  ticks(psg, 4);
  CHECK(near(psg.samples()[0], 0.25f) && near(psg.samples()[1], 0.0f));
  psg.clearSamples();
  psg.writeStereo(0x00);  // Everything muted.
  ticks(psg, 4);
  CHECK(near(psg.samples()[0], 0.0f) && near(psg.samples()[1], 0.0f));
}

void testSampleRateAndFilter() {
  SegaPSG psg;
  psg.setSampleRate(48000);
  setTone(psg, 0, 0x100);
  setVolume(psg, 0, 0);
  psg.step(3579545);  // One second of CPU time.
  const long frames = static_cast<long>(psg.frameCount());
  CHECK(frames >= 47990 && frames <= 48010);
  CHECK(psg.samples().size() == psg.frameCount() * 2);

  // With the high-pass on, a constant level decays towards zero.
  SegaPSG dc;
  dc.setSampleRate(48000);
  setTone(dc, 0, 0);
  setVolume(dc, 0, 0);
  dc.step(3579545 / 2);
  const auto &s = dc.samples();
  CHECK(s[0] > 0.1f);
  CHECK(std::fabs(s[s.size() - 2]) < 0.01f);

  psg.clearSamples();
  CHECK(psg.frameCount() == 0);
}

void testReset() {
  SegaPSG psg;
  setTone(psg, 0, 0x155);
  setVolume(psg, 0, 0);
  psg.reset();
  CHECK(psg.getState().tonePeriod[0] == 0);
  CHECK(psg.getState().volume[0] == 15);
  CHECK(psg.getState().lfsr == 0x8000);
}

void testState() {
  SegaPSG psg;
  exactRig(psg);
  setTone(psg, 1, 0x0AB);
  setVolume(psg, 1, 2);
  psg.write(0xE6);
  ticks(psg, 100);
  SegaPSGState s = psg.getState();
  nlohmann::json j = s;
  SegaPSGState r = j.get<SegaPSGState>();
  CHECK(r.tonePeriod == s.tonePeriod && r.volume == s.volume);
  CHECK(r.lfsr == s.lfsr && r.counter == s.counter && r.flip == s.flip);

  // A restored chip continues exactly like the original.
  SegaPSG copy;
  exactRig(copy);
  copy.setState(r);
  psg.clearSamples();
  ticks(psg, 50);
  ticks(copy, 50);
  CHECK(psg.samples() == copy.samples());
}

}  // namespace

int main() {
  testRegisterProtocol();
  testSquareWave();
  testPeriodTwoIsFastestSquare();
  testSamplePlaybackPeriodZero();
  testVolumeCurve();
  testChannelsMix();
  testPeriodicNoise();
  testNoiseFollowsTone2();
  testWhiteNoiseFeedback();
  testGameGearStereo();
  testSampleRateAndFilter();
  testReset();
  testState();
  if (failures == 0) std::printf("sega_psg: all checks passed\n");
  return failures == 0 ? 0 : 1;
}
