#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "emulators/console/Console.hh"
#include "emulators/console/atari2600/Atari2600.hh"
#include "emulators/console/chip8/Chip8.hh"
#include "emulators/console/game_boy/GameBoy.hh"

using namespace emscripten;

/**
 * JavaScript-facing wrapper around a Console.
 * Pointers returned by framebufferPtr()/audioPtr() point into WASM memory:
 * read them with Module.HEAPU8 / Module.HEAPF32 right after stepFrame().
 */
class EmulatorWrapper {
 public:
  bool load(const std::string& type, const val& rom) {
    if (type == "chip8") {
      console_ = std::make_unique<Chip8>();
    } else if (type == "atari2600") {
      console_ = std::make_unique<Atari2600>();
    } else if (type == "gameboy") {
      console_ = std::make_unique<GameBoy>();
    } else {
      return false;  // add "atari2600", ... here
    }
    if (sampleRate_ > 0) console_->setAudioSampleRate(sampleRate_);
    const auto bytes = convertJSArrayToNumberVector<uint8_t>(rom);
    return console_->loadProgram(bytes.data(), bytes.size());
  }

  void setSampleRate(int hz) {
    sampleRate_ = hz;
    if (console_) console_->setAudioSampleRate(hz);
  }

  void reset() {
    if (console_) console_->reset();
  }
  void stepFrame() {
    if (console_) console_->stepFrame();
  }
  void setKey(int key, bool pressed) {
    if (console_) console_->setKey(key, pressed);
  }
  bool isHalted() const { return !console_ || console_->isHalted(); }

  int width() const { return console_ ? console_->width() : 0; }
  int height() const { return console_ ? console_->height() : 0; }
  uintptr_t framebufferPtr() const {
    return console_ ? reinterpret_cast<uintptr_t>(console_->framebuffer()) : 0;
  }

  int audioFrameCount() const {
    return console_ ? static_cast<int>(console_->audioFrameCount()) : 0;
  }
  uintptr_t audioPtr() const {
    return console_ ? reinterpret_cast<uintptr_t>(console_->audioSamples()) : 0;
  }

 private:
  std::unique_ptr<Console> console_;
  int sampleRate_ = 0;
};

EMSCRIPTEN_BINDINGS(emulators) {
  class_<EmulatorWrapper>("Emulator")
      .constructor<>()
      .function("load", &EmulatorWrapper::load)
      .function("setSampleRate", &EmulatorWrapper::setSampleRate)
      .function("reset", &EmulatorWrapper::reset)
      .function("stepFrame", &EmulatorWrapper::stepFrame)
      .function("setKey", &EmulatorWrapper::setKey)
      .function("isHalted", &EmulatorWrapper::isHalted)
      .function("width", &EmulatorWrapper::width)
      .function("height", &EmulatorWrapper::height)
      .function("framebufferPtr", &EmulatorWrapper::framebufferPtr)
      .function("audioFrameCount", &EmulatorWrapper::audioFrameCount)
      .function("audioPtr", &EmulatorWrapper::audioPtr);
}
