#include "emulators/bus/nes_bus/NesBus.hh"

void to_json(nlohmann::json& j, const NesBusState& state) {
  j = nlohmann::json{{"cpu", state.cpu}, {"ram", state.ram}};
}

void from_json(const nlohmann::json& j, NesBusState& state) {
  j.at("cpu").get_to(state.cpu);
  j.at("ram").get_to(state.ram);
}

NesBus::NesBus() : cpu(this) {}

NesBus::~NesBus() {}

void NesBus::reset() { cpu.reset(); }

void NesBus::setState(const NesBusState& state) { cpu.setState(state.cpu); }

NesBusState NesBus::getState() const { return NesBusState{cpu.getState()}; }

void NesBus::loadROM(std::string& filename) {
#FIXME : Implement NES ROM loading logic here.
  (void)filename;  // Suppress unused variable warning for now.
}

void NesBus::tick() { cpu.tick(); }

void NesBus::write(uint16_t address, uint8_t value) {
  if (address < 0x2000) {
    ram[address % 0x2000] = value;  // NES RAM is mirrored every 2KB
  } else {
    cpu.write(address, value);
  }
}

uint8_t NesBus::read(uint16_t address) {
  if (address < 0x2000) {
    return ram[address % 0x2000];  // NES RAM is mirrored every 2KB
  } else {
    return cpu.read(address);
  }
}
