#include "atari2600_bus.hh"
#include <algorithm>
#include <cstddef>
#include <stdexcept>

Atari2600Bus::Atari2600Bus() {
    this->cpu = new CPU6507();
    this->tia = new TIA();
    this->riot = new RIOT();
    this->cartridge = new CartridgeAtari2600();
    this->lastTIAMappedValues.fill(0);
    this->lastRIOTMappedValues.fill(0);
}

Atari2600Bus::~Atari2600Bus() {
    delete this->cpu;
    delete this->tia;
    delete this->riot;
    delete this->cartridge;
}

uint8_t Atari2600Bus::read(uint16_t address) {
    address &= 0x1FFF;

    if (address >= 0x1000) {
        if (this->cartridge->size() == 0) {
            return 0xFF;
        }
        return this->cartridge->read((address - 0x1000) % this->cartridge->size());
    } else if (address < 0x0080) {
        return this->tia->read(address & 0x3F);
    } else if (address >= 0x0080 && address < 0x0100) {
        return this->riot->read(address - 0x0080);
    } else {
        return this->cpu->memory[address];
    }
}

void Atari2600Bus::loadCartridge(const std::string& path) {
    if (!this->cartridge->load(path)) {
        throw std::runtime_error("Failed to load cartridge: " + path);
    }

    this->cpu->reset();
    this->tia->reset();
    this->riot->reset();

    this->mapCartridgeToCPUMemory();
    this->syncComponentsToCPUMemory();
    this->syncCPUMemoryToComponents();
}

void Atari2600Bus::write(uint16_t address, uint8_t value) {
    address &= 0x1FFF;

    if (address >= 0x1000) {
        return;
    } else if (address < 0x0080) {
        this->tia->write(address & 0x3F, value);
    } else if (address >= 0x0080 && address < 0x0100) {
        this->riot->write(address - 0x0080, value);
    } else {
        this->cpu->memory[address] = value;
    }
}

void Atari2600Bus::tick() {
    this->syncComponentsToCPUMemory();
    this->cpu->step();
    this->syncCPUMemoryToComponents();

    this->tia->tick();
    this->tia->tick();
    this->tia->tick();

    if (this->tia->isFrameReady()) {
        this->tia->renderFrame();
    }
}

void Atari2600Bus::setRenderer(SDL_Renderer* renderer) {
    this->tia->setRenderer(renderer);
}

void Atari2600Bus::mapCartridgeToCPUMemory() {
    std::fill(this->cpu->memory.begin(), this->cpu->memory.end(), 0);

    const std::size_t romSize = this->cartridge->size();
    if (romSize == 0) {
        return;
    }

    for (uint16_t addr = 0x1000; addr <= 0x1FFF; ++addr) {
        const uint16_t romAddress = static_cast<uint16_t>((addr - 0x1000) % romSize);
        this->cpu->memory[addr] = this->cartridge->read(romAddress);
    }

    this->cpu->PC = 0x1000;
}

void Atari2600Bus::syncComponentsToCPUMemory() {
    for (uint16_t i = 0; i < 64; ++i) {
        this->cpu->memory[i] = this->tia->read(static_cast<uint8_t>(i));
    }

    for (uint16_t i = 0; i < 128; ++i) {
        this->cpu->memory[0x0080 + i] = this->riot->read(i);
    }
}

void Atari2600Bus::syncCPUMemoryToComponents() {
    for (uint8_t i = 0; i < 64; ++i) {
        const uint8_t value = this->cpu->memory[i];
        if (this->lastTIAMappedValues[i] != value) {
            this->tia->write(i, value);
            this->lastTIAMappedValues[i] = value;
        }
    }

    for (uint16_t i = 0; i < 128; ++i) {
        const uint8_t value = this->cpu->memory[0x0080 + i];
        if (this->lastRIOTMappedValues[i] != value) {
            this->riot->write(i, value);
            this->lastRIOTMappedValues[i] = value;
        }
    }
}