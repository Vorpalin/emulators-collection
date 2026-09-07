#include <cstdint>
#include "CPU6507.hh"
#include "riot.hh"
#include "TIA.hh"
#include "cartridge_atari2600.hh"

class Atari2600Bus {
public:
    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t value);

private:
    CPU6507* cpu;
    TIA* tia;
    RIOT* riot;
    CartridgeAtari2600* cartridge;
};