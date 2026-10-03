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
 * @brief WebAssembly bridge exposing the emulator core to JavaScript.
 *
 * EmulatorWrapper provides the interface between the C++ emulator
 * implementations and the web frontend through Emscripten bindings.
 *
 * It is responsible for:
 * - Instantiating the requested emulator.
 * - Loading ROM data received from JavaScript.
 * - Forwarding input, reset and frame execution requests.
 * - Exposing the video framebuffer to JavaScript.
 * - Exposing generated audio samples to JavaScript.
 * - Configuring the audio sample rate.
 *
 * The underlying emulator is owned through a std::unique_ptr and can be
 * replaced when loading a different console type.
 *
 * Supported emulator types are:
 * - `"chip8"`: CHIP-8.
 * - `"atari2600"`: Atari 2600.
 * - `"gameboy"`: Game Boy.
 *
 * @note The pointers returned by framebufferPtr() and audioPtr() are
 *       addresses in the WebAssembly linear memory. They must be interpreted
 *       by the JavaScript side using the appropriate typed array.
 */
class EmulatorWrapper {
 public:
  /**
   * @brief Creates and loads an emulator from a JavaScript ROM buffer.
   *
   * The emulator implementation is selected from @p type. The ROM data
   * contained in @p rom is converted from a JavaScript array to a C++ byte
   * vector before being passed to the emulator.
   *
   * If an audio sample rate has previously been configured with
   * setSampleRate(), it is applied to the newly created emulator.
   *
   * @param type Emulator type to instantiate.
   * @param rom JavaScript array containing the ROM binary data.
   *
   * @return true if the emulator type is supported and the ROM was loaded
   *         successfully, false otherwise.
   *
   * @retval false If @p type does not correspond to a supported emulator.
   * @retval false If the selected emulator fails to load the ROM.
   */
  bool load(const std::string& type, const val& rom) {
    if (type == "chip8") {
      console_ = std::make_unique<Chip8>();
    } else if (type == "atari2600") {
      console_ = std::make_unique<Atari2600>();
    } else if (type == "gameboy") {
      console_ = std::make_unique<GameBoy>();
    } else {
      return false;
    }
    if (sampleRate_ > 0) console_->setAudioSampleRate(sampleRate_);
    const auto bytes = convertJSArrayToNumberVector<uint8_t>(rom);
    return console_->loadProgram(bytes.data(), bytes.size());
  }

  /**
   * @brief Configures the audio sample rate.
   *
   * The configured sample rate is stored so that it can also be applied to
   * emulators loaded after this call.
   *
   * @param hz Audio sample rate in Hertz.
   */
  void setSampleRate(int hz) {
    sampleRate_ = hz;
    if (console_) console_->setAudioSampleRate(hz);
  }

  /**
   * @brief Resets the currently loaded emulator.
   *
   * Does nothing if no emulator has been loaded.
   */
  void reset() {
    if (console_) console_->reset();
  }

  /**
   * @brief Executes one emulation frame.
   *
   * Does nothing if no emulator has been loaded.
   */
  void stepFrame() {
    if (console_) console_->stepFrame();
  }

  /**
   * @brief Updates the state of an emulator input.
   *
   * @param key Key identifier understood by the currently loaded emulator.
   * @param pressed true if the key is pressed, false if it is released.
   */
  void setKey(int key, bool pressed) {
    if (console_) console_->setKey(key, pressed);
  }

  /**
   * @brief Checks whether the current emulator has halted.
   *
   * An unloaded emulator is considered halted.
   *
   * @return true if no emulator is loaded or if the current emulator is
   *         halted, false otherwise.
   */
  bool isHalted() const { return !console_ || console_->isHalted(); }

  /**
   * @brief Returns the width of the current framebuffer.
   *
   * @return Framebuffer width in pixels, or 0 if no emulator is loaded.
   */
  int width() const { return console_ ? console_->width() : 0; }

  /**
   * @brief Returns the height of the current framebuffer.
   *
   * @return Framebuffer height in pixels, or 0 if no emulator is loaded.
   */
  int height() const { return console_ ? console_->height() : 0; }

  /**
   * @brief Returns the address of the current video framebuffer.
   *
   * The returned value is a byte address in WebAssembly linear memory.
   * JavaScript can use this address to create a typed array view over the
   * framebuffer.
   *
   * @return WebAssembly memory address of the framebuffer, or 0 if no
   *         emulator is loaded.
   *
   * @warning The returned address is only valid while the underlying
   *          framebuffer remains allocated at that location.
   */
  uintptr_t framebufferPtr() const {
    return console_ ? reinterpret_cast<uintptr_t>(console_->framebuffer()) : 0;
  }

  /**
   * @brief Returns the number of audio frames currently available.
   *
   * @return Number of audio frames, or 0 if no emulator is loaded.
   */
  int audioFrameCount() const {
    return console_ ? static_cast<int>(console_->audioFrameCount()) : 0;
  }

  /**
   * @brief Returns the address of the generated audio samples.
   *
   * The returned value is a byte address in WebAssembly linear memory.
   * JavaScript can use this address to access the generated audio samples.
   *
   * @return WebAssembly memory address of the audio buffer, or 0 if no
   *         emulator is loaded.
   *
   * @warning The returned address is only valid while the underlying audio
   *          buffer remains allocated at that location.
   */
  uintptr_t audioPtr() const {
    return console_ ? reinterpret_cast<uintptr_t>(console_->audioSamples()) : 0;
  }

 private:
  /**
   * @brief Currently loaded emulator instance.
   *
   * Ownership is exclusive and automatically managed through std::unique_ptr.
   */
  std::unique_ptr<Console> console_;

  /**
   * @brief Audio sample rate to apply to the current and future emulators.
   *
   * A value of 0 indicates that no explicit sample rate has been configured.
   */
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
