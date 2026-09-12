#include "atari2600_bus.hh"
#include <stdexcept>

Atari2600Bus::Atari2600Bus() {
    this->cpu = new CPU6507();
    this->tia = new TIA();
    this->riot = new RIOT();
    this->cartridge = new CartridgeAtari2600();
}

Atari2600Bus::~Atari2600Bus() {
    delete this->cpu;
    delete this->tia;
    delete this->riot;
    delete this->cartridge;
}

uint8_t Atari2600Bus::read(uint16_t address) {
    if (address < 0x1000) {
        return this->cartridge->read(address);
    } else if (address >= 0x1000 && address < 0x1800) {
        return this->riot->read(address - 0x1000);
    } else if (address >= 0x1800 && address < 0x1C00) {
        return this->tia->read(address - 0x1800);
    } else {
        // Handle invalid address
        throw std::out_of_range("Invalid read address: " + std::to_string(address));
    }
}

void Atari2600Bus::loadCartridge(const std::string& path) {
    if (!this->cartridge->load(path)) {
        // Handle cartridge loading error
        throw std::runtime_error("Failed to load cartridge: " + path);
    }
}

void Atari2600Bus::write(uint16_t address, uint8_t value) {
    if (address < 0x1000) {
        // Writing to cartridge ROM is typically not allowed
        throw std::runtime_error("Attempted to write to cartridge ROM at address: " + std::to_string(address));
    } else if (address >= 0x1000 && address < 0x1800) {
        this->riot->write(address - 0x1000, value);
    } else if (address >= 0x1800 && address < 0x1C00) {
        this->tia->write(address - 0x1800, value);
    } else {
        // Handle invalid address
        throw std::out_of_range("Invalid write address: " + std::to_string(address));
    }
}

void Atari2600Bus::tick() {
    // Advance the CPU, TIA, and RIOT components by one tick
    this->cpu->step();
    this->tia->tick();
}

void Atari2600Bus::setRenderer(SDL_Renderer* renderer) {
    // Set the SDL renderer for TIA rendering
    // This function can be used to pass the SDL_Renderer to the TIA for rendering graphics
    // For example, you might store the renderer in the TIA class and use it during rendering
    this->tia->setRenderer(renderer);
}