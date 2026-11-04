#include "emulators/cartridge/nes_cartridge/NesCartridge.hh"

#include <stdexcept>
#include <vector>

#include "emulators/cartridge/nes_cartridge/mapper/Mapper000.hh"
#include "emulators/cartridge/nes_cartridge/mapper/Mapper001.hh"
#include "emulators/cartridge/nes_cartridge/mapper/Mapper002.hh"
#include "emulators/cartridge/nes_cartridge/mapper/Mapper003.hh"
#include "emulators/cartridge/nes_cartridge/mapper/Mapper004.hh"
#include "emulators/cartridge/nes_cartridge/mapper/Mapper007.hh"
#include "emulators/cartridge/nes_cartridge/mapper/Mapper066.hh"

namespace {

constexpr size_t kHeaderSize = 16;
constexpr size_t kTrainerSize = 512;
constexpr size_t kPrgUnit = 0x4000;  // 16 KiB
constexpr size_t kChrUnit = 0x2000;  // 8 KiB

/// Sequential reader over an in-memory ROM image.
class Reader {
 public:
  Reader(const uint8_t* data, size_t size) : data_(data), size_(size) {}

  std::vector<uint8_t> block(size_t n, const char* what) {
    if (n > size_ - pos_) {
      throw std::runtime_error(std::string("ROM truncated while reading ") +
                               what);
    }
    std::vector<uint8_t> out(data_ + pos_, data_ + pos_ + n);
    pos_ += n;
    return out;
  }
  void skip(size_t n) { pos_ = (n > size_ - pos_) ? size_ : pos_ + n; }

 private:
  const uint8_t* data_;
  size_t size_;
  size_t pos_ = 0;
};

std::unique_ptr<Mapper> MakeMapper(int id, RomData rom) {
  switch (id) {
    case 0:
      return std::make_unique<Mapper000>(std::move(rom));
    case 1:
      return std::make_unique<Mapper001>(std::move(rom));
    case 2:
      return std::make_unique<Mapper002>(std::move(rom));
    case 3:
      return std::make_unique<Mapper003>(std::move(rom));
    case 4:
      return std::make_unique<Mapper004>(std::move(rom));
    case 7:
      return std::make_unique<Mapper007>(std::move(rom));
    case 66:
      return std::make_unique<Mapper066>(std::move(rom));
    default:
      throw std::runtime_error("Unsupported mapper: " + std::to_string(id));
  }
}

}  // namespace

bool NesCartridge::load(const uint8_t* data, std::size_t size) {
  mapper_.reset();
  error_.clear();
  try {
    Reader in(data, size);
    const std::vector<uint8_t> h = in.block(kHeaderSize, "header");
    if (h[0] != 'N' || h[1] != 'E' || h[2] != 'S' || h[3] != 0x1A) {
      throw std::runtime_error("Not an iNES file (bad magic)");
    }

    const bool nes2 = (h[7] & 0x0C) == 0x08;
    size_t prg_units = h[4];
    size_t chr_units = h[5];
    // Old dumps carry garbage ("DiskDude!") in bytes 7-15: ignore the upper
    // mapper nibble when it is present.
    const bool dirty = !nes2 && (h[12] | h[13] | h[14] | h[15]) != 0;
    int id = (h[6] >> 4) | (dirty ? 0 : (h[7] & 0xF0));

    if (nes2) {
      if ((h[9] & 0x0F) == 0x0F || (h[9] >> 4) == 0x0F) {
        throw std::runtime_error("NES 2.0 exponent sizes unsupported");
      }
      prg_units |= static_cast<size_t>(h[9] & 0x0F) << 8;
      chr_units |= static_cast<size_t>(h[9] >> 4) << 8;
      id |= (h[8] & 0x0F) << 8;
    }
    if (prg_units == 0) throw std::runtime_error("ROM has no PRG data");

    RomData rom;
    if (h[6] & 0x08) {
      rom.mirroring = NesMirroring::kFourScreen;
    } else {
      rom.mirroring =
          (h[6] & 0x01) ? NesMirroring::kVertical : NesMirroring::kHorizontal;
    }

    if (h[6] & 0x04) in.skip(kTrainerSize);  // trainer, unused

    rom.prg_rom = in.block(prg_units * kPrgUnit, "PRG-ROM");
    if (chr_units == 0) {
      rom.chr_is_ram = true;
      rom.chr.assign(kChrUnit, 0);
    } else {
      rom.chr = in.block(chr_units * kChrUnit, "CHR-ROM");
    }

    mapper_ = MakeMapper(id, std::move(rom));
    mapper_id_ = id;
    has_battery_ = (h[6] & 0x02) != 0;
    mapper_->Reset();
    return true;
  } catch (const std::exception& e) {
    error_ = e.what();
    mapper_.reset();
    return false;
  }
}

nlohmann::json NesCartridge::saveState() const {
  nlohmann::json j = nlohmann::json::object();
  if (mapper_) mapper_->SaveState(j);
  return j;
}

void NesCartridge::loadState(const nlohmann::json& j) {
  if (mapper_) mapper_->LoadState(j);
}
