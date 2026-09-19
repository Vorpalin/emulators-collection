#include "CPU65.hh"
#include "opcodes.hh"

#include <iostream>

CPU65::CPU65(bool isCPU6502) {
    reset(isCPU6502);
}

void CPU65::reset(bool isCPU6502) {
    A = 0;
    X = 0;
    Y = 0;
    SP = 0x0100; // Stack Pointer starts at 0x0100
    PC = 0xFFFC; // Reset vector address
    C = 0;
    Z = 0;
    I = 0;
    D = 0;
    B = 0;
    V = 0;
    N = 0;

    if (isCPU6502) {
        // Initialize memory for 6502 CPU
        memory.resize(65536, 0); // 64KB of memory
    } else {
        // Initialize memory for 6507 CPU
        memory.resize(8192, 0); // 8KB of memory
    }

    for (auto& byte : memory) {
        byte = 0; // Clear memory
    }
}

uint8_t CPU65::fetch(uint32_t &cycles) {
    uint8_t instruction = memory[PC];
    PC++;
    cycles--;
    return instruction;
}

uint16_t CPU65::fetch16(uint32_t &cycles) {
    uint16_t lowByte = fetch(cycles);
    uint16_t highByte = fetch(cycles);
    return (highByte << 8) | lowByte;
}

uint8_t CPU65::readMemory(uint32_t &cycles, uint16_t address) {
    uint8_t data = memory[address];
    cycles--;
    return data;
}

void CPU65::zeroPageAddX(uint32_t &cycles, uint16_t &address) {
    address = (address + X) & 0xFF; // Wrap around for zero page
    cycles--;
}

void CPU65::zeroPageAddY(uint32_t &cycles, uint16_t &address) {
    address = (address + Y) & 0xFF; // Wrap around for zero page
    cycles--;
}

void CPU65::ldaSetFlags() {
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}

void CPU65::execute(uint32_t cycles) {
    while (cycles > 0) {
        uint8_t instruction = fetch(cycles);
        
        switch (instruction) {
            case INS_LDA_IMM: // LDA Immediate
                uint8_t addr = fetch(cycles);
                A = addr;
                ldaSetFlags();
                break;
            case INS_LDA_ZP: // LDA Zero Page
                uint8_t addr = fetch(cycles);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            case INS_LDA_ZPX: // LDA Zero Page,X
                uint8_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            case INS_LDA_ABS: // LDA Absolute
                uint16_t addr = fetch16(cycles);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            case INS_LDA_ABSX: // LDA Absolute,X
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            case INS_LDA_ABSY: // LDA Absolute,Y
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            case INS_LDA_INDX: // LDA (Indirect,X)
                uint8_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            case INS_LDA_INDY: // LDA (Indirect),Y
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            default:
                std::cerr << "Unknown instruction: " << std::hex << static_cast<int>(instruction) << std::endl;
                return;
        }
    }
}