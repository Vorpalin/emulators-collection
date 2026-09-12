#include "TIA.hh"

TIA::TIA() {
    this->reset();
}

void TIA::reset() {
    for (auto& reg : registers) {
        reg = 0; // Initialize all registers to 0
    }
    scanline = 0;
    cycle = 0;
}

void TIA::write(uint8_t reg, uint8_t value) {
    if (reg < sizeof(registers)) {
        registers[reg] = value; // Write the value to the specified register
    }
}

uint8_t TIA::read(uint8_t reg) {
    if (reg < sizeof(registers)) {
        return registers[reg]; // Return the value of the specified register
    } else {
        return 0xFF; // Return 0xFF for out-of-bounds registers
    }
}

void TIA::tick() {
    cycle++;
    if (cycle >= 76) {
        cycle = 0;
        scanline++;
    }
}

void TIA::setRenderer(SDL_Renderer* renderer) {
    this->renderer = renderer; // Set the SDL renderer for rendering graphics
}