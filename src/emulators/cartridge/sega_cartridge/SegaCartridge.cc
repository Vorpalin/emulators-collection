#include "emulators/cartridge/sega_cartridge/SegaCartridge.hh"

#include <algorithm>

SegaCartridge::SegaCartridge() { state_.ram.assign(kRamSize, 0); }

bool SegaCartridge::load(const uint8_t *data, std::size_t size) {
  error_.clear();
  rom_.clear();
  if (data == nullptr || size == 0) {
    error_ = "empty ROM image";
    return false;
  }
  // Drop the 512-byte header that some copiers prepend.
  if (size % kPageSize == 512) {
    data += 512;
    size -= 512;
  }
  if (size > 0x400000) {
    error_ = "ROM image larger than 4 MiB";
    return false;
  }
  pages_ = std::max<std::size_t>(1, (size + kPageSize - 1) / kPageSize);
  rom_.assign(pages_ * kPageSize, 0xFF);
  std::copy(data, data + size, rom_.begin());

  mapper_ = detectCodemasters() ? Mapper::Codemasters : Mapper::Sega;
  state_.ram.assign(kRamSize, 0);
  dirty_ = false;
  reset();
  return true;
}

bool SegaCartridge::detectCodemasters() const {
  if (rom_.size() < 0x8000) return false;
  auto word = [&](std::size_t o) {
    return static_cast<uint32_t>(rom_[o] | (rom_[o + 1] << 8));
  };
  const uint32_t checksum = word(0x7FE6);
  const uint32_t complement = word(0x7FE8);
  if (checksum == 0 || ((checksum + complement) & 0xFFFF) != 0) return false;
  uint32_t sum = 0;
  for (std::size_t o = 0; o + 1 < rom_.size(); o += 2) {
    if (o >= 0x7FE0 && o < 0x7FF0) continue;  // The header itself is excluded.
    sum += word(o);
  }
  return (sum & 0xFFFF) == checksum;
}

void SegaCartridge::reset() {
  state_.bank = {0, 1, 2};
  state_.ramControl = 0;
}

uint8_t SegaCartridge::read(uint16_t address) const {
  if (rom_.empty()) return 0xFF;
  if (address < 0x4000) {
    if (mapper_ == Mapper::Sega && address < 0x400) return rom_[address];
    return rom_[page(0) * kPageSize + address];
  }
  if (address < 0x8000) return rom_[page(1) * kPageSize + (address - 0x4000u)];
  if (address >= 0xC000) return 0xFF;
  if (ramMapped()) return state_.ram[ramOffset(address)];
  return rom_[page(2) * kPageSize + (address - 0x8000u)];
}

void SegaCartridge::write(uint16_t address, uint8_t value) {
  if (mapper_ == Mapper::Codemasters) {
    if (address == 0x0000) state_.bank[0] = value;
    else if (address == 0x4000) state_.bank[1] = value;
    else if (address == 0x8000) state_.bank[2] = value;
    return;
  }
  if (address >= 0x8000 && address < 0xC000 && ramMapped()) {
    uint8_t &cell = state_.ram[ramOffset(address)];
    if (cell != value) {
      cell = value;
      dirty_ = true;
    }
  }
}

void SegaCartridge::writeMapper(uint16_t address, uint8_t value) {
  if (mapper_ != Mapper::Sega) return;
  switch (address) {
    case 0xFFFC: state_.ramControl = value; break;
    case 0xFFFD: state_.bank[0] = value; break;
    case 0xFFFE: state_.bank[1] = value; break;
    case 0xFFFF: state_.bank[2] = value; break;
    default: break;
  }
}

bool SegaCartridge::loadBatteryRam(const uint8_t *data, std::size_t size) {
  if (!loaded() || data == nullptr) return false;
  if (size != kRamSize && size != kRamSize / 2) return false;
  std::fill(state_.ram.begin(), state_.ram.end(), 0);
  std::copy(data, data + size, state_.ram.begin());
  dirty_ = false;
  return true;
}

void SegaCartridge::setState(const SegaCartridgeState &s) {
  state_ = s;
  if (state_.ram.size() != kRamSize) state_.ram.resize(kRamSize, 0);
}
