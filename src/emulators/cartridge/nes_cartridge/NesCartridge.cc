#include "emulators/cartridge/nes_cartridge/NesCartridge.hh"

#include <array>
#include <fstream>
#include <stdexcept>

#include "emulators/cartridge/nes_cartridge/Mapper000.hh"
#include "emulators/cartridge/nes_cartridge/Mapper001.hh"
#include "emulators/cartridge/nes_cartridge/Mapper002.hh"
#include "emulators/cartridge/nes_cartridge/Mapper003.hh"

namespace nes {

namespace {

constexpr size_t kHeaderSize = 16;
constexpr size_t kTrainerSize = 512;
constexpr size_t kPrgUnit = 0x4000;  // 16 KiB
constexpr size_t kChrUnit = 0x2000;  // 8 KiB

std::vector<uint8_t> ReadBlock(std::istream& in, size_t size,
                               const char* what) {
  std::vector<uint8_t> data(size);
  in.read(reinterpret_cast<char*>(data.data()),
          static_cast<std::streamsize>(size));
  if (static_cast<size_t>(in.gcount()) != size) {
    throw std::runtime_error(std::string("ROM truncated while reading ") +
                             what);
  }
  return data;
}

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
    default:
      throw std::runtime_error("Unsupported mapper: " + std::to_string(id));
  }
}

}  // namespace

NesCartridge::NesCartridge() {}

void NesCartridge::load(std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("Cannot open ROM file: " + path);
  }
  const std::vector<uint8_t> h = ReadBlock(in, kHeaderSize, "header");
  if (h[0] != 'N' || h[1] != 'E' || h[2] != 'S' || h[3] != 0x1A) {
    throw std::runtime_error("Not an iNES file (bad magic)");
  }

  const bool nes2 = (h[7] & 0x0C) == 0x08;
  size_t prg_units = h[4];
  size_t chr_units = h[5];
  mapper_id_ = (h[7] & 0xF0) | (h[6] >> 4);

  if (nes2) {
    if ((h[9] & 0x0F) == 0x0F || (h[9] >> 4) == 0x0F) {
      throw std::runtime_error("NES 2.0 exponent-multiplier sizes unsupported");
    }
    prg_units |= static_cast<size_t>(h[9] & 0x0F) << 8;
    chr_units |= static_cast<size_t>(h[9] >> 4) << 8;
    mapper_id_ |= (h[8] & 0x0F) << 8;
  }
  if (prg_units == 0) throw std::runtime_error("ROM has no PRG data");

  has_battery_ = (h[6] & 0x02) != 0;

  RomData rom;
  if (h[6] & 0x08) {
    rom.mirroring = Mirroring::kFourScreen;
  } else {
    rom.mirroring =
        (h[6] & 0x01) ? Mirroring::kVertical : Mirroring::kHorizontal;
  }

  if (h[6] & 0x04) in.ignore(kTrainerSize);  // trainer, unused

  rom.prg_rom = ReadBlock(in, prg_units * kPrgUnit, "PRG-ROM");
  if (chr_units == 0) {
    rom.chr_is_ram = true;
    rom.chr.assign(kChrUnit, 0);
  } else {
    rom.chr = ReadBlock(in, chr_units * kChrUnit, "CHR-ROM");
  }

  mapper_ = MakeMapper(mapper_id_, std::move(rom));
}

void Cartridge::LoadSave(const std::string& path) {
  if (!has_battery_) return;
  std::ifstream f(path, std::ios::binary);
  if (!f) return;  // first run: no save yet
  auto& ram = mapper_->PrgRam();
  f.read(reinterpret_cast<char*>(ram.data()),
         static_cast<std::streamsize>(ram.size()));
}

void Cartridge::WriteSave(const std::string& path) const {
  if (!has_battery_) return;
  std::ofstream f(path, std::ios::binary | std::ios::trunc);
  if (!f) throw std::runtime_error("Cannot write save file: " + path);
  const auto& ram = mapper_->PrgRam();
  f.write(reinterpret_cast<const char*>(ram.data()),
          static_cast<std::streamsize>(ram.size()));
}

}  // namespace nes
