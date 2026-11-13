#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "emulators/console/Console.hh"
#include "emulators/console/atari2600/Atari2600.hh"
#include "emulators/console/chip8/Chip8.hh"
#include "emulators/console/game_boy/GameBoy.hh"
#include "emulators/console/nes/Nes.hh"

using namespace emscripten;

namespace {

/**
 * @brief Copies a JavaScript byte array into a C++ vector.
 *
 * The bytes are copied in a single `TypedArray.set()` call through a view over
 * the vector's storage. This is much faster than
 * convertJSArrayToNumberVector(), which crosses the JavaScript/WebAssembly
 * boundary once per element (a multi-megabyte Game Boy ROM means millions of
 * crossings).
 *
 * @param data A `Uint8Array` (or any array-like of byte values).
 *
 * @return The copied bytes. Empty if @p data is empty.
 */
std::vector<uint8_t> toBytes(const val& data) {
  const auto length = data["length"].as<std::size_t>();
  std::vector<uint8_t> bytes(length);
  if (length > 0) {
    val(typed_memory_view(length, bytes.data())).call<void>("set", data);
  }
  return bytes;
}

}  // namespace

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
 * - `"nes"`: Nintendo Entertainment System.
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
   * contained in @p rom is copied to a C++ byte vector before being passed to
   * the emulator.
   *
   * If an audio sample rate has previously been configured with
   * setSampleRate(), it is applied to the newly created emulator.
   *
   * The new emulator only replaces the current one once the ROM has been
   * accepted. If loading fails, the previously loaded emulator (if any) is
   * left untouched, so a rejected ROM can never leave a half-initialised
   * machine behind that stepFrame() would then run.
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
    std::unique_ptr<Console> console;
    if (type == "chip8") {
      console = std::make_unique<Chip8>();
    } else if (type == "atari2600") {
      console = std::make_unique<Atari2600>();
    } else if (type == "gameboy") {
      console = std::make_unique<GameBoy>();
    } else if (type == "nes") {
      console = std::make_unique<Nes>();
    } else {
      return false;
    }
    if (sampleRate_ > 0) console->setAudioSampleRate(sampleRate_);
    const auto bytes = toBytes(rom);
    if (!console->loadProgram(bytes.data(), bytes.size())) return false;
    console_ = std::move(console);
    return true;
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

  bool loadState(std::string const& json) {
    if (console_) return console_->loadState(json);
    return false;
  }

  std::string saveState() const {
    if (console_) return console_->saveState();
    return "";
  }

  /**
   * @brief Tells whether the loaded cartridge has battery-backed RAM.
   *
   * Only such cartridges (Game Boy, NES) keep an "in-game save".
   */
  bool hasBattery() const { return console_ && console_->hasBattery(); }

  /**
   * @brief Copies the battery RAM into a new JavaScript Uint8Array.
   *
   * The returned array owns its data (it does not alias WebAssembly memory),
   * so it stays valid after the emulator keeps running.
   */
  val saveRam() const {
    const std::vector<uint8_t> ram =
        console_ ? console_->saveRam() : std::vector<uint8_t>{};
    const val view(typed_memory_view(ram.size(), ram.data()));
    return val::global("Uint8Array").new_(view);  // copies the bytes
  }

  /**
   * @brief Restores battery RAM previously returned by saveRam().
   * @retval false If there is no battery or the size does not match.
   */
  bool loadRam(const val& data) {
    if (!console_) return false;
    const auto bytes = toBytes(data);
    return console_->loadRam(bytes.data(), bytes.size());
  }

  /**
   * @brief True if the game changed its battery RAM since the last call.
   *
   * This is how the front-end notices that the game has saved.
   */
  bool consumeRamDirty() { return console_ && console_->consumeRamDirty(); }

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
      .function("audioPtr", &EmulatorWrapper::audioPtr)
      .function("loadState", &EmulatorWrapper::loadState)
      .function("saveState", &EmulatorWrapper::saveState)
      .function("hasBattery", &EmulatorWrapper::hasBattery)
      .function("saveRam", &EmulatorWrapper::saveRam)
      .function("loadRam", &EmulatorWrapper::loadRam)
      .function("consumeRamDirty", &EmulatorWrapper::consumeRamDirty);
}
