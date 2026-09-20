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

void CPU65::writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) {
    memory[address] = value;
    cycles--;
}

void CPU65::zeroPageAddX(uint32_t &cycles, uint16_t &address) {
    address = (address + X);
    cycles--;
}

void CPU65::zeroPageAddY(uint32_t &cycles, uint16_t &address) {
    address = (address + Y);
    cycles--;
}

void CPU65::ldaSetFlags() {
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}

void CPU65::adcSetFlags(uint8_t value, uint16_t result) {
    C = (result > 0xFF) ? 1 : 0; // Set Carry flag
    V = ((A ^ result) & (value ^ result) & 0x80) ? 1 : 0; // Set Overflow flag
    
    A = static_cast<uint8_t>(result);
    
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}

void CPU65::andSetFlags() {
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}

void CPU65::aslSetFlags(uint32_t &cycles) {
    C = (A & 0x80) ? 1 : 0; // Set Carry flag
    A <<= 1; // Shift left
    --cycles; // Decrement cycles for the operation
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}

void CPU65::bitSetFlags(uint8_t value)
{
    Z = (A & value) == 0; // Set Zero flag
    N = (value & 0x80) != 0; // Set Negative flag
    V = (value & 0x40) != 0; // Set Overflow flag
}

void CPU65::cmpSetFlags(uint8_t value)
{
    uint16_t result = static_cast<uint16_t>(A) - static_cast<uint16_t>(value);
    C = (A >= value) ? 1 : 0; // Set Carry flag
    Z = (result == 0); // Set Zero flag
    N = (result & 0x80) != 0; // Set Negative flag
}

void CPU65::execute(uint32_t cycles) {
    while (cycles > 0) {
        uint8_t instruction = fetch(cycles);
        
        switch (instruction) {
            case INS_LDA_IMM: // LDA Immediate
            {
                uint8_t addr = fetch(cycles);
                A = addr;
                ldaSetFlags();
                break;
            }
            case INS_LDA_ZP: // LDA Zero Page
            {
                uint8_t addr = fetch(cycles);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_LDA_ZPX: // LDA Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_LDA_ABS: // LDA Absolute
            {
                uint16_t addr = fetch16(cycles);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_LDA_ABSX: // LDA Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_LDA_ABSY: // LDA Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_LDA_INDX: // LDA (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_LDA_INDY: // LDA (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                A = this->readMemory(cycles, addr);
                ldaSetFlags();
                break;
            }
            case INS_ADC_IMM: // ADC Immediate
            {
                uint8_t value = fetch(cycles);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_ZP: // ADC Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_ZPX: // ADC Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_ABS: // ADC Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_ABSX: // ADC Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_ABSY: // ADC Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_INDX: // ADC (Indirect,X)
            {
                uint16_t zpAddr = fetch16(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_ADC_INDY: // ADC (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A + value + C;
                adcSetFlags(value, result);
                break;
            }
            case INS_AND_IMM: // AND Immediate
            {
                uint8_t value = fetch(cycles);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_ZP: // AND Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_ZPX: // AND Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_ABS: // AND Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_ABSX: // AND Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_ABSY: // AND Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_INDX: // AND (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_AND_INDY: // AND (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A &= value;
                andSetFlags();
                break;
            }
            case INS_ASL_ACC: // ASL Accumulator
            {
                aslSetFlags(cycles);
                break;
            }
            case INS_ASL_ZP: // ASL Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                aslSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ASL_ZPX: // ASL Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                aslSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ASL_ABS: // ASL Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                aslSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ASL_ABSX: // ASL Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                aslSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_BCC: // BCC (Branch if Carry Clear)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (C == 0) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BCS: // BCS (Branch if Carry Set)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (C == 1) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BEQ: // BEQ (Branch if Equal)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (Z == 1) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BIT_ZP: // BIT Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                bitSetFlags(value);
                break;
            }
            case INS_BIT_ABS: // BIT Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                bitSetFlags(value);
                break;
            }
            case INS_BMI: // BMI (Branch if Minus)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (N == 1) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BNE: // BNE (Branch if Not Equal)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (Z == 0) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BPL: // BPL (Branch if Positive)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (N == 0) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BRK: // BRK (Force Interrupt)
            {
                // FIXME: Implement BRK instruction behavior (interrupt handling)
                B = 1; // Set Break flag
                std::cerr << "BRK instruction encountered. Stopping execution." << std::endl;
                return;
            }
            case INS_BVC: // BVC (Branch if Overflow Clear)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (V == 0) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_BVS: // BVS (Branch if Overflow Set)
            {
                int8_t offset = static_cast<int8_t>(fetch(cycles));
                if (V == 1) {
                    PC += offset;
                    cycles--; // Branch taken, decrement cycles
                }
                break;
            }
            case INS_CLC: // CLC (Clear Carry Flag)
            {
                C = 0; // Clear Carry flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_CLD: // CLD (Clear Decimal Mode)
            {
                D = 0; // Clear Decimal Mode flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_CLI: // CLI (Clear Interrupt Disable)
            {
                I = 0; // Clear Interrupt Disable flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_CLV: // CLV (Clear Overflow Flag)
            {
                V = 0; // Clear Overflow flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_CMP_IMM: // CMP Immediate
            {
                uint8_t value = fetch(cycles);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_ZP: // CMP Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_ZPX: // CMP Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_ABS: // CMP Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_ABSX: // CMP Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_ABSY: // CMP Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_INDX: // CMP (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            case INS_CMP_INDY: // CMP (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                cmpSetFlags(value);
                break;
            }
            default:
            {
                std::cerr << "Unknown instruction: " << std::hex << static_cast<int>(instruction) << std::endl;
                return;
            }
        }
    }
}