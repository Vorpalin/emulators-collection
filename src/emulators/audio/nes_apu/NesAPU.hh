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
inline void to_json(nlohmann::json& j, const Envelope& v) {
  j = nlohmann::json{{"start", v.start},       {"loop", v.loop},
                     {"constant", v.constant}, {"period", v.period},
                     {"divider", v.divider},   {"decay", v.decay}};
}

inline void from_json(const nlohmann::json& j, Envelope& v) {
  j.at("start").get_to(v.start);
  j.at("loop").get_to(v.loop);
  j.at("constant").get_to(v.constant);
  j.at("period").get_to(v.period);
  j.at("divider").get_to(v.divider);
  j.at("decay").get_to(v.decay);
}
inline void to_json(nlohmann::json& j, const Pulse& v) {
  j = nlohmann::json{{"channel1", v.channel1},
                     {"enabled", v.enabled},
                     {"env", v.env},
                     {"duty", v.duty},
                     {"seq", v.seq},
                     {"timer", v.timer},
                     {"reload", v.reload},
                     {"length", v.length},
                     {"sweep_enable", v.sweep_enable},
                     {"sweep_negate", v.sweep_negate},
                     {"sweep_reload", v.sweep_reload},
                     {"sweep_period", v.sweep_period},
                     {"sweep_shift", v.sweep_shift},
                     {"sweep_divider", v.sweep_divider}};
}

inline void from_json(const nlohmann::json& j, Pulse& v) {
  j.at("channel1").get_to(v.channel1);
  j.at("enabled").get_to(v.enabled);
  j.at("env").get_to(v.env);
  j.at("duty").get_to(v.duty);
  j.at("seq").get_to(v.seq);
  j.at("timer").get_to(v.timer);
  j.at("reload").get_to(v.reload);
  j.at("length").get_to(v.length);
  j.at("sweep_enable").get_to(v.sweep_enable);
  j.at("sweep_negate").get_to(v.sweep_negate);
  j.at("sweep_reload").get_to(v.sweep_reload);
  j.at("sweep_period").get_to(v.sweep_period);
  j.at("sweep_shift").get_to(v.sweep_shift);
  j.at("sweep_divider").get_to(v.sweep_divider);
}
inline void to_json(nlohmann::json& j, const Triangle& v) {
  j = nlohmann::json{{"enabled", v.enabled},
                     {"control", v.control},
                     {"linear_reload_flag", v.linear_reload_flag},
                     {"linear_reload", v.linear_reload},
                     {"linear", v.linear},
                     {"timer", v.timer},
                     {"reload", v.reload},
                     {"length", v.length},
                     {"seq", v.seq}};
}

inline void from_json(const nlohmann::json& j, Triangle& v) {
  j.at("enabled").get_to(v.enabled);
  j.at("control").get_to(v.control);
  j.at("linear_reload_flag").get_to(v.linear_reload_flag);
  j.at("linear_reload").get_to(v.linear_reload);
  j.at("linear").get_to(v.linear);
  j.at("timer").get_to(v.timer);
  j.at("reload").get_to(v.reload);
  j.at("length").get_to(v.length);
  j.at("seq").get_to(v.seq);
}
inline void to_json(nlohmann::json& j, const Noise& v) {
  j = nlohmann::json{{"enabled", v.enabled}, {"env", v.env},
                     {"mode", v.mode},       {"timer", v.timer},
                     {"period", v.period},   {"length", v.length},
                     {"shift", v.shift}};
}

inline void from_json(const nlohmann::json& j, Noise& v) {
  j.at("enabled").get_to(v.enabled);
  j.at("env").get_to(v.env);
  j.at("mode").get_to(v.mode);
  j.at("timer").get_to(v.timer);
  j.at("period").get_to(v.period);
  j.at("length").get_to(v.length);
  j.at("shift").get_to(v.shift);
}
inline void to_json(nlohmann::json& j, const Dmc& v) {
  j = nlohmann::json{{"enabled", v.enabled},
                     {"irq_enable", v.irq_enable},
                     {"loop", v.loop},
                     {"irq", v.irq},
                     {"period", v.period},
                     {"timer", v.timer},
                     {"level", v.level},
                     {"sample_addr", v.sample_addr},
                     {"sample_length", v.sample_length},
                     {"cur_addr", v.cur_addr},
                     {"bytes_remaining", v.bytes_remaining},
                     {"buffer", v.buffer},
                     {"buffer_empty", v.buffer_empty},
                     {"shifter", v.shifter},
                     {"bits_remaining", v.bits_remaining},
                     {"silence", v.silence}};
}

inline void from_json(const nlohmann::json& j, Dmc& v) {
  j.at("enabled").get_to(v.enabled);
  j.at("irq_enable").get_to(v.irq_enable);
  j.at("loop").get_to(v.loop);
  j.at("irq").get_to(v.irq);
  j.at("period").get_to(v.period);
  j.at("timer").get_to(v.timer);
  j.at("level").get_to(v.level);
  j.at("sample_addr").get_to(v.sample_addr);
  j.at("sample_length").get_to(v.sample_length);
  j.at("cur_addr").get_to(v.cur_addr);
  j.at("bytes_remaining").get_to(v.bytes_remaining);
  j.at("buffer").get_to(v.buffer);
  j.at("buffer_empty").get_to(v.buffer_empty);
  j.at("shifter").get_to(v.shifter);
  j.at("bits_remaining").get_to(v.bits_remaining);
  j.at("silence").get_to(v.silence);
}
}  // namespace nesapu

inline void to_json(nlohmann::json& j, const NesAPUState& v) {
  j = nlohmann::json{{"p1", v.p1},
                     {"p2", v.p2},
                     {"tri", v.tri},
                     {"noise", v.noise},
                     {"dmc", v.dmc},
                     {"frame_cycle", v.frame_cycle},
                     {"five_step", v.five_step},
                     {"irq_inhibit", v.irq_inhibit},
                     {"frame_irq", v.frame_irq},
                     {"odd_cycle", v.odd_cycle},
                     {"phase", v.phase},
                     {"acc", v.acc},
                     {"acc_count", v.acc_count},
                     {"hp_prev_in", v.hp_prev_in},
                     {"hp_prev_out", v.hp_prev_out}};
}

inline void from_json(const nlohmann::json& j, NesAPUState& v) {
  j.at("p1").get_to(v.p1);
  j.at("p2").get_to(v.p2);
  j.at("tri").get_to(v.tri);
  j.at("noise").get_to(v.noise);
  j.at("dmc").get_to(v.dmc);
  j.at("frame_cycle").get_to(v.frame_cycle);
  j.at("five_step").get_to(v.five_step);
  j.at("irq_inhibit").get_to(v.irq_inhibit);
  j.at("frame_irq").get_to(v.frame_irq);
  j.at("odd_cycle").get_to(v.odd_cycle);
  j.at("phase").get_to(v.phase);
  j.at("acc").get_to(v.acc);
  j.at("acc_count").get_to(v.acc_count);
  j.at("hp_prev_in").get_to(v.hp_prev_in);
  j.at("hp_prev_out").get_to(v.hp_prev_out);
}
