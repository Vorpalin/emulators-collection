#include "Atari2600Bus.hh"

#include <string>

Atari2600Bus::Atari2600Bus() : cpu(this), cartbridge(), mos6532(), tia1a() {
    // Initialize the Atari 2600 bus
}

void Atari2600Bus::loadROM(std::string &filename) {
    cartbridge.loadROM(filename);
}

uint8_t Atari2600Bus::readMemory(uint16_t address)
{
    address &= 0x1FFF; // 6507 has a 13-bit address bus

    if (address < 0x0080)
        return tia1a.read(address);

    else if (address < 0x0100)
        return mos6532.read(address - 0x0080);

    else if (address >= 0x1000)
        return cartbridge.read(address - 0x1000);

    return 0;
}

void Atari2600Bus::writeMemory(uint16_t address, uint8_t value)
{
    address &= 0x1FFF;

    if (address < 0x0080)
        tia1a.write(address, value);
    else if (address < 0x0100)
        mos6532.write(address - 0x0080, value);
}

void Atari2600Bus::reset() {
    cpu.reset(); // Reset the CPU
    cartbridge.reset(); // Reset the Cartbridge
    mos6532.reset(); // Reset the MOS6532
    tia1a.reset(); // Reset the TIA1A
}

void Atari2600Bus::tick() {
    // Advance the bus by one clock cycle
    uint32_t cpuCycles = cpu.execute();

    for (uint32_t i = 0; i < cpuCycles * 3; ++i)
        tia1a.tick();

    mos6532.update(cpuCycles);
}