#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

/**
 * @file NesController.hh
 * @brief Standard NES gamepad: 8 buttons read serially through $4016/$4017.
 */

/** One standard NES controller and its 8-bit shift register. */
class NesController {
 public:
  /// Button bit positions, in the order the console shifts them out.
  enum Button : uint8_t {
    kA = 0,
    kB = 1,
    kSelect = 2,
    kStart = 3,
    kUp = 4,
    kDown = 5,
    kLeft = 6,
    kRight = 7,
  };

  void reset() { buttons_ = shift_ = 0, strobe_ = false; }

  /// Press or release a button (host side).
  void setButton(Button b, bool pressed) {
    const uint8_t mask = static_cast<uint8_t>(1u << b);
    buttons_ = pressed ? static_cast<uint8_t>(buttons_ | mask)
                       : static_cast<uint8_t>(buttons_ & ~mask);
    if (strobe_) shift_ = buttons_;
  }

  /// Write to $4016 (bit 0 = strobe).
  void write(uint8_t value) {
    strobe_ = (value & 1) != 0;
    if (strobe_) shift_ = buttons_;
  }

  /// Read one bit from $4016/$4017.
  uint8_t read() {
    const uint8_t bit = shift_ & 1;
    if (strobe_) {
      shift_ = buttons_;
    } else {
      shift_ = static_cast<uint8_t>((shift_ >> 1) | 0x80);  // 1s after 8 reads
    }
    return bit;
  }

  uint8_t buttons_ = 0;
  uint8_t shift_ = 0;
  bool strobe_ = false;
};

inline void to_json(nlohmann::json& j, const NesController& v) {
  j = nlohmann::json{
      {"buttons_", v.buttons_}, {"shift_", v.shift_}, {"strobe_", v.strobe_}};
}

inline void from_json(const nlohmann::json& j, NesController& v) {
  j.at("buttons_").get_to(v.buttons_);
  j.at("shift_").get_to(v.shift_);
  j.at("strobe_").get_to(v.strobe_);
}
