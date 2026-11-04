#include "emulators/audio/nes_apu/NesAPU.hh"

#include <cmath>

namespace nesapu {
namespace {

constexpr uint8_t kLengthTable[32] = {
    10, 254, 20, 2,  40, 4,  80, 6,  160, 8,  60, 10, 14, 12, 26, 14,
    12, 16,  24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30};

constexpr uint8_t kDuty[4][8] = {{0, 1, 0, 0, 0, 0, 0, 0},
                                 {0, 1, 1, 0, 0, 0, 0, 0},
                                 {0, 1, 1, 1, 1, 0, 0, 0},
                                 {1, 0, 0, 1, 1, 1, 1, 1}};

constexpr uint16_t kNoisePeriod[16] = {
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068};

constexpr uint16_t kDmcPeriod[16] = {428, 380, 340, 320, 286, 254, 226, 214,
                                     190, 160, 142, 128, 106, 84,  72,  54};

constexpr uint8_t kTriangle[32] = {15, 14, 13, 12, 11, 10, 9,  8,  7,  6, 5,
                                   4,  3,  2,  1,  0,  0,  1,  2,  3,  4, 5,
                                   6,  7,  8,  9,  10, 11, 12, 13, 14, 15};

}  // namespace

void Envelope::clock() {
  if (start) {
    start = false;
    decay = 15;
    divider = period;
  } else if (divider == 0) {
    divider = period;
    if (decay > 0) {
      --decay;
    } else if (loop) {
      decay = 15;
    }
  } else {
    --divider;
  }
}

uint16_t Pulse::sweepTarget() const {
  const int change = reload >> sweep_shift;
  int target = reload;
  if (sweep_negate) {
    target -= change + (channel1 ? 1 : 0);
  } else {
    target += change;
  }
  return static_cast<uint16_t>(target < 0 ? 0 : target);
}

void Pulse::clockSweep() {
  if (sweep_divider == 0 && sweep_enable && sweep_shift > 0 && reload >= 8 &&
      sweepTarget() <= 0x7FF) {
    reload = sweepTarget();
  }
  if (sweep_divider == 0 || sweep_reload) {
    sweep_divider = sweep_period;
    sweep_reload = false;
  } else {
    --sweep_divider;
  }
}

void Pulse::clockTimer() {
  if (timer == 0) {
    timer = reload;
    seq = static_cast<uint8_t>((seq + 1) & 7);
  } else {
    --timer;
  }
}

uint8_t Pulse::output() const {
  if (!enabled || length == 0 || reload < 8 || sweepTarget() > 0x7FF ||
      kDuty[duty][seq] == 0) {
    return 0;
  }
  return env.volume();
}

void Triangle::clockTimer() {
  if (timer == 0) {
    timer = reload;
    if (length > 0 && linear > 0) seq = static_cast<uint8_t>((seq + 1) & 31);
  } else {
    --timer;
  }
}

void Triangle::clockLinear() {
  if (linear_reload_flag) {
    linear = linear_reload;
  } else if (linear > 0) {
    --linear;
  }
  if (!control) linear_reload_flag = false;
}

uint8_t Triangle::output() const { return kTriangle[seq]; }

void Noise::clockTimer() {
  if (timer == 0) {
    timer = period;
    const uint16_t bit = (shift >> (mode ? 6 : 1)) & 1;
    const uint16_t feedback = (shift & 1) ^ bit;
    shift = static_cast<uint16_t>((shift >> 1) | (feedback << 14));
  } else {
    --timer;
  }
}

uint8_t Noise::output() const {
  if (!enabled || length == 0 || (shift & 1)) return 0;
  return env.volume();
}

}  // namespace nesapu

using namespace nesapu;

NesAPU::NesAPU() {
  for (size_t n = 1; n < pulse_table_.size(); ++n) {
    pulse_table_[n] =
        static_cast<float>(95.52 / (8128.0 / static_cast<double>(n) + 100.0));
  }
  for (size_t n = 1; n < tnd_table_.size(); ++n) {
    tnd_table_[n] =
        static_cast<float>(163.67 / (24329.0 / static_cast<double>(n) + 100.0));
  }
  setSampleRate(48000);
  reset();
}

void NesAPU::reset() {
  state_ = NesAPUState{};
  state_.p1.channel1 = true;
  state_.noise.shift = 1;
  state_.dmc.timer = state_.dmc.period;
  samples_.clear();
}

void NesAPU::setSampleRate(int hz) {
  if (hz <= 0) return;
  cycles_per_sample_ = 1789773.0 / static_cast<double>(hz);
  // One-pole high-pass at ~90 Hz (removes the DC offset like the console).
  const double rc = 1.0 / (2.0 * 3.14159265358979 * 90.0);
  const double dt = 1.0 / static_cast<double>(hz);
  hp_alpha_ = rc / (rc + dt);
  state_.phase = state_.acc = state_.acc_count = 0.0;
}

float NesAPU::mix() const {
  const size_t p = static_cast<size_t>(state_.p1.output() + state_.p2.output());
  const size_t t = static_cast<size_t>(
      3 * state_.tri.output() + 2 * state_.noise.output() + state_.dmc.level);
  return pulse_table_[p] + tnd_table_[t];
}

void NesAPU::clockQuarterFrame() {
  state_.p1.env.clock();
  state_.p2.env.clock();
  state_.noise.env.clock();
  state_.tri.clockLinear();
}

void NesAPU::clockHalfFrame() {
  for (Pulse* p : {&state_.p1, &state_.p2}) {
    if (!p->env.loop && p->length > 0) --p->length;
    p->clockSweep();
  }
  if (!state_.tri.control && state_.tri.length > 0) --state_.tri.length;
  if (!state_.noise.env.loop && state_.noise.length > 0) --state_.noise.length;
}

void NesAPU::dmcFetch() {
  Dmc& d = state_.dmc;
  if (!d.buffer_empty || d.bytes_remaining == 0 || !read_mem_) return;
  d.buffer = read_mem_(d.cur_addr);  // the bus accounts for the CPU stall
  d.buffer_empty = false;
  d.cur_addr = static_cast<uint16_t>((d.cur_addr + 1) | 0x8000);
  if (--d.bytes_remaining == 0) {
    if (d.loop) {
      d.cur_addr = d.sample_addr;
      d.bytes_remaining = d.sample_length;
    } else if (d.irq_enable) {
      d.irq = true;
    }
  }
}

void NesAPU::clockDmc() {
  Dmc& d = state_.dmc;
  if (!d.silence) {
    if (d.shifter & 1) {
      if (d.level <= 125) d.level = static_cast<uint8_t>(d.level + 2);
    } else if (d.level >= 2) {
      d.level = static_cast<uint8_t>(d.level - 2);
    }
    d.shifter >>= 1;
  }
  if (--d.bits_remaining == 0) {
    d.bits_remaining = 8;
    if (d.buffer_empty) {
      d.silence = true;
    } else {
      d.silence = false;
      d.shifter = d.buffer;
      d.buffer_empty = true;
      dmcFetch();
    }
  }
}

void NesAPU::tickCycle() {
  NesAPUState& s = state_;

  // Frame sequencer (CPU-cycle thresholds of the 4/5-step sequences).
  ++s.frame_cycle;
  switch (s.frame_cycle) {
    case 7457:
      clockQuarterFrame();
      break;
    case 14913:
      clockQuarterFrame();
      clockHalfFrame();
      break;
    case 22371:
      clockQuarterFrame();
      break;
    case 29828:
      if (!s.five_step && !s.irq_inhibit) s.frame_irq = true;
      break;
    case 29829:
      if (!s.five_step) {
        clockQuarterFrame();
        clockHalfFrame();
        if (!s.irq_inhibit) s.frame_irq = true;
      }
      break;
    case 29830:
      if (!s.five_step) {
        if (!s.irq_inhibit) s.frame_irq = true;
        s.frame_cycle = 0;
      }
      break;
    case 37281:
      clockQuarterFrame();
      clockHalfFrame();
      break;
    case 37282:
      s.frame_cycle = 0;
      break;
    default:
      break;
  }

  // Timers: triangle and noise every CPU cycle, pulses every other one.
  s.tri.clockTimer();
  s.noise.clockTimer();
  if (s.odd_cycle) {
    s.p1.clockTimer();
    s.p2.clockTimer();
  }
  s.odd_cycle = !s.odd_cycle;

  Dmc& d = s.dmc;
  if (d.timer > 0) --d.timer;
  if (d.timer == 0) {
    d.timer = d.period;
    clockDmc();
  }
}

void NesAPU::step(uint32_t cpuCycles) {
  for (uint32_t i = 0; i < cpuCycles; ++i) {
    tickCycle();

    // Box-filter down-sampling of the CPU-rate signal.
    state_.acc += static_cast<double>(mix());
    state_.acc_count += 1.0;
    state_.phase += 1.0;
    if (state_.phase >= cycles_per_sample_) {
      state_.phase -= cycles_per_sample_;
      const double in = state_.acc / state_.acc_count;
      state_.acc = state_.acc_count = 0.0;
      const double out =
          hp_alpha_ * (state_.hp_prev_out + in - state_.hp_prev_in);
      state_.hp_prev_in = in;
      state_.hp_prev_out = out;
      samples_.push_back(static_cast<float>(out));
    }
  }
}

uint8_t NesAPU::read(uint16_t addr) {
  if (addr != 0x4015) return 0;
  const NesAPUState& s = state_;
  uint8_t v = 0;
  if (s.p1.length > 0) v |= 0x01;
  if (s.p2.length > 0) v |= 0x02;
  if (s.tri.length > 0) v |= 0x04;
  if (s.noise.length > 0) v |= 0x08;
  if (s.dmc.bytes_remaining > 0) v |= 0x10;
  if (s.frame_irq) v |= 0x40;
  if (s.dmc.irq) v |= 0x80;
  state_.frame_irq = false;
  return v;
}

void NesAPU::write(uint16_t addr, uint8_t v) {
  NesAPUState& s = state_;
  switch (addr) {
    case 0x4000:
    case 0x4004: {
      Pulse& p = addr == 0x4000 ? s.p1 : s.p2;
      p.duty = v >> 6;
      p.env.loop = (v & 0x20) != 0;
      p.env.constant = (v & 0x10) != 0;
      p.env.period = v & 0x0F;
      break;
    }
    case 0x4001:
    case 0x4005: {
      Pulse& p = addr == 0x4001 ? s.p1 : s.p2;
      p.sweep_enable = (v & 0x80) != 0;
      p.sweep_period = (v >> 4) & 7;
      p.sweep_negate = (v & 0x08) != 0;
      p.sweep_shift = v & 7;
      p.sweep_reload = true;
      break;
    }
    case 0x4002:
    case 0x4006: {
      Pulse& p = addr == 0x4002 ? s.p1 : s.p2;
      p.reload = static_cast<uint16_t>((p.reload & 0x700) | v);
      break;
    }
    case 0x4003:
    case 0x4007: {
      Pulse& p = addr == 0x4003 ? s.p1 : s.p2;
      p.reload = static_cast<uint16_t>((p.reload & 0x0FF) | ((v & 7) << 8));
      if (p.enabled) p.length = kLengthTable[v >> 3];
      p.seq = 0;
      p.env.start = true;
      break;
    }
    case 0x4008:
      s.tri.control = (v & 0x80) != 0;
      s.tri.linear_reload = v & 0x7F;
      break;
    case 0x400A:
      s.tri.reload = static_cast<uint16_t>((s.tri.reload & 0x700) | v);
      break;
    case 0x400B:
      s.tri.reload =
          static_cast<uint16_t>((s.tri.reload & 0x0FF) | ((v & 7) << 8));
      if (s.tri.enabled) s.tri.length = kLengthTable[v >> 3];
      s.tri.linear_reload_flag = true;
      break;
    case 0x400C:
      s.noise.env.loop = (v & 0x20) != 0;
      s.noise.env.constant = (v & 0x10) != 0;
      s.noise.env.period = v & 0x0F;
      break;
    case 0x400E:
      s.noise.mode = (v & 0x80) != 0;
      s.noise.period = kNoisePeriod[v & 0x0F];
      break;
    case 0x400F:
      if (s.noise.enabled) s.noise.length = kLengthTable[v >> 3];
      s.noise.env.start = true;
      break;
    case 0x4010:
      s.dmc.irq_enable = (v & 0x80) != 0;
      s.dmc.loop = (v & 0x40) != 0;
      s.dmc.period = kDmcPeriod[v & 0x0F];
      if (!s.dmc.irq_enable) s.dmc.irq = false;
      break;
    case 0x4011:
      s.dmc.level = v & 0x7F;
      break;
    case 0x4012:
      s.dmc.sample_addr = static_cast<uint16_t>(0xC000 | (v << 6));
      break;
    case 0x4013:
      s.dmc.sample_length = static_cast<uint16_t>((v << 4) | 1);
      break;
    case 0x4015:
      s.p1.enabled = (v & 0x01) != 0;
      s.p2.enabled = (v & 0x02) != 0;
      s.tri.enabled = (v & 0x04) != 0;
      s.noise.enabled = (v & 0x08) != 0;
      if (!s.p1.enabled) s.p1.length = 0;
      if (!s.p2.enabled) s.p2.length = 0;
      if (!s.tri.enabled) s.tri.length = 0;
      if (!s.noise.enabled) s.noise.length = 0;
      s.dmc.irq = false;
      if (v & 0x10) {
        if (s.dmc.bytes_remaining == 0) {
          s.dmc.cur_addr = s.dmc.sample_addr;
          s.dmc.bytes_remaining = s.dmc.sample_length;
          dmcFetch();
        }
      } else {
        s.dmc.bytes_remaining = 0;
      }
      break;
    case 0x4017:
      s.five_step = (v & 0x80) != 0;
      s.irq_inhibit = (v & 0x40) != 0;
      if (s.irq_inhibit) s.frame_irq = false;
      s.frame_cycle = 0;
      if (s.five_step) {
        clockQuarterFrame();
        clockHalfFrame();
      }
      break;
    default:
      break;
  }
}
