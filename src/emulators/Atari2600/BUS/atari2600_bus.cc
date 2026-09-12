#include "atari2600_bus.hh"
#include <algorithm>
#include <cstddef>
#include <stdexcept>


Atari2600Bus::Atari2600Bus(CPU6507* cpu) {
    this->cpu = cpu;
    this->tia = new TIA();
    this->riot = new RIOT();
    this->cartridge = new CartridgeAtari2600();
}

Atari2600Bus::~Atari2600Bus() {
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
    this->cpu->step();

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