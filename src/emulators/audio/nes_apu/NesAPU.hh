#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <nlohmann/json.hpp>
#include <vector>

/**
 * @file NesAPU.hh
 * @brief NES audio processing unit (Ricoh 2A03): two pulse channels, a
 *        triangle, a noise channel and a delta-modulation channel (DMC).
 */

namespace nesapu {

struct Envelope {
  bool start = false;
  bool loop = false;  ///< Also the length-counter halt flag.
  bool constant = false;
  uint8_t period = 0;  ///< Volume when constant, else divider period.
  uint8_t divider = 0;
  uint8_t decay = 0;

  void clock();
  uint8_t volume() const { return constant ? period : decay; }
};

struct Pulse {
  bool channel1 = false;  ///< Sweep negation differs between pulse 1 and 2.
  bool enabled = false;
  Envelope env;
  uint8_t duty = 0;
  uint8_t seq = 0;
  uint16_t timer = 0;
  uint16_t reload = 0;
  uint8_t length = 0;
  bool sweep_enable = false;
  bool sweep_negate = false;
  bool sweep_reload = false;
  uint8_t sweep_period = 0;
  uint8_t sweep_shift = 0;
  uint8_t sweep_divider = 0;

  uint16_t sweepTarget() const;
  void clockSweep();
  void clockTimer();
  uint8_t output() const;
};

struct Triangle {
  bool enabled = false;
  bool control = false;  ///< Length halt / linear counter control.
  bool linear_reload_flag = false;
  uint8_t linear_reload = 0;
  uint8_t linear = 0;
  uint16_t timer = 0;
  uint16_t reload = 0;
  uint8_t length = 0;
  uint8_t seq = 0;

  void clockTimer();
  void clockLinear();
  uint8_t output() const;
};

struct Noise {
  bool enabled = false;
  Envelope env;
  bool mode = false;
  uint16_t timer = 0;
  uint16_t period = 4;
  uint8_t length = 0;
  uint16_t shift = 1;

  void clockTimer();
  uint8_t output() const;
};

struct Dmc {
  bool enabled = false;
  bool irq_enable = false;
  bool loop = false;
  bool irq = false;
  uint16_t period = 428;
  uint16_t timer = 428;
  uint8_t level = 0;
  uint16_t sample_addr = 0xC000;
  uint16_t sample_length = 1;
  uint16_t cur_addr = 0xC000;
  uint16_t bytes_remaining = 0;
  uint8_t buffer = 0;
  bool buffer_empty = true;
  uint8_t shifter = 0;
  uint8_t bits_remaining = 8;
  bool silence = true;
};

}  // namespace nesapu

/** Serializable APU state. */
struct NesAPUState {
  nesapu::Pulse p1, p2;
  nesapu::Triangle tri;
  nesapu::Noise noise;
  nesapu::Dmc dmc;
  uint32_t frame_cycle = 0;
  bool five_step = false;
  bool irq_inhibit = false;
  bool frame_irq = false;
  bool odd_cycle = false;
  double phase = 0.0;
  double acc = 0.0;
  double acc_count = 0.0;
  double hp_prev_in = 0.0;
  double hp_prev_out = 0.0;
};

class NesAPU {
 public:
  using MemoryReader = std::function<uint8_t(uint16_t)>;

  NesAPU();

  void reset();

  /// Memory access used by the DMC to fetch samples.
  void setMemoryReader(MemoryReader reader) { read_mem_ = std::move(reader); }

  /// Set the output sample rate (mono samples are produced at this rate).
  void setSampleRate(int hz);

  /// Advance the APU by @p cpuCycles CPU cycles.
  void step(uint32_t cpuCycles);

  /// Read $4015.
  uint8_t read(uint16_t addr);
  /// Write $4000-$4013, $4015 or $4017.
  void write(uint16_t addr, uint8_t value);

  bool irqPending() const { return state_.frame_irq || state_.dmc.irq; }

  /// Mono samples produced since the last clearSamples().
  const std::vector<float>& samples() const { return samples_; }
  void clearSamples() { samples_.clear(); }

  NesAPUState getState() const { return state_; }
  void setState(const NesAPUState& s) { state_ = s; }

 private:
  void tickCycle();
  void clockQuarterFrame();
  void clockHalfFrame();
  void clockDmc();
  void dmcFetch();
  float mix() const;

  NesAPUState state_;
  MemoryReader read_mem_;
  double cycles_per_sample_ = 1789773.0 / 48000.0;
  double hp_alpha_ = 0.0;
  std::vector<float> samples_;
  std::array<float, 31> pulse_table_{};
  std::array<float, 203> tnd_table_{};
};

namespace nesapu {
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Envelope, start, loop, constant, period,
                                   divider, decay)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Pulse, channel1, enabled, env, duty, seq,
                                   timer, reload, length, sweep_enable,
                                   sweep_negate, sweep_reload, sweep_period,
                                   sweep_shift, sweep_divider)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Triangle, enabled, control,
                                   linear_reload_flag, linear_reload, linear,
                                   timer, reload, length, seq)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Noise, enabled, env, mode, timer, period,
                                   length, shift)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Dmc, enabled, irq_enable, loop, irq, period,
                                   timer, level, sample_addr, sample_length,
                                   cur_addr, bytes_remaining, buffer,
                                   buffer_empty, shifter, bits_remaining,
                                   silence)
}  // namespace nesapu

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(NesAPUState, p1, p2, tri, noise, dmc,
                                   frame_cycle, five_step, irq_inhibit,
                                   frame_irq, odd_cycle, phase, acc, acc_count,
                                   hp_prev_in, hp_prev_out)
