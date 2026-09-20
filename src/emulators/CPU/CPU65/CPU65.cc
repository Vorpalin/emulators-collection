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
            case INS_CPX_IMM: // CPX Immediate
            {
                uint8_t value = fetch(cycles);
                cpxSetFlags(value);
                break;
            }
            case INS_CPX_ZP: // CPX Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                cpxSetFlags(value);
                break;
            }
            case INS_CPX_ABS: // CPX Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                cpxSetFlags(value);
                break;
            }
            case INS_CPY_IMM: // CPY Immediate
            {
                uint8_t value = fetch(cycles);
                cpySetFlags(value);
                break;
            }
            case INS_CPY_ZP: // CPY Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                cpySetFlags(value);
                break;
            }
            case INS_CPY_ABS: // CPY Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                cpySetFlags(value);
                break;
            }
            case INS_DEC_ZP: // DEC Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                value--;
                --cycles; // Decrement cycles for the operation
                decSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_DEC_ZPX: // DEC Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                value--;
                --cycles; // Decrement cycles for the operation
                decSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_DEC_ABS: // DEC Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                value--;
                --cycles; // Decrement cycles for the operation
                decSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_DEC_ABSX: // DEC Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                value--;
                --cycles; // Decrement cycles for the operation
                decSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_DEX: // DEX (Decrement X Register)
            {
                X--;
                --cycles; // Decrement cycles for the operation
                dexSetFlags();
                break;
            }
            case INS_DEY: // DEY (Decrement Y Register)
            {
                Y--;
                --cycles; // Decrement cycles for the operation
                deySetFlags();
                break;
            }
            case INS_EOR_IMM: // EOR Immediate
            {
                uint8_t value = fetch(cycles);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_ZP: // EOR Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_ZPX: // EOR Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_ABS: // EOR Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_ABSX: // EOR Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_ABSY: // EOR Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_INDX: // EOR (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_EOR_INDY: // EOR (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A ^= value;
                eorSetFlags();
                break;
            }
            case INS_INC_ZP: // INC Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                value++;
                --cycles; // Decrement cycles for the operation
                incSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_INC_ZPX: // INC Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                value++;
                --cycles; // Decrement cycles for the operation
                incSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_INC_ABS: // INC Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                value++;
                --cycles; // Decrement cycles for the operation
                incSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_INC_ABSX: // INC Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                value++;
                --cycles; // Decrement cycles for the operation
                incSetFlags(value);
                writeMemory(cycles, addr, value);
                break;
            }
            case INS_INX: // INX (Increment X Register)
            {
                X++;
                --cycles; // Decrement cycles for the operation
                inxSetFlags();
                break;
            }
            case INS_INY: // INY (Increment Y Register)
            {
                Y++;
                --cycles; // Decrement cycles for the operation
                inySetFlags();
                break;
            }
            case INS_JMP_ABS: // JMP Absolute
            {
                uint16_t addr = fetch16(cycles);
                PC = addr;
                break;
            }
            case INS_JMP_IND: // JMP Indirect
            {
                uint16_t addr = fetch16(cycles);
                uint16_t targetAddr = this->readMemory(cycles, addr) | (this->readMemory(cycles, (addr + 1) & 0xFFFF) << 8);
                PC = targetAddr;
                break;
            }
            case INS_JSR: // JSR (Jump to Subroutine)
            {
                uint16_t addr = fetch16(cycles);
                uint16_t returnAddr = PC - 1; // Address of the next instruction after JSR
                --cycles; // Decrement cycles for the operation
                writeMemory(cycles, SP--, (returnAddr >> 8) & 0xFF); // Push high byte
                writeMemory(cycles, SP--, returnAddr & 0xFF);        // Push low byte
                PC = addr;
                break;
            }
            case INS_LDX_IMM: // LDX Immediate
            {
                uint8_t value = fetch(cycles);
                X = value;
                ldxSetFlags();
                break;
            }
            case INS_LDX_ZP: // LDX Zero Page
            {
                uint8_t addr = fetch(cycles);
                X = this->readMemory(cycles, addr);
                ldxSetFlags();
                break;
            }
            case INS_LDX_ZPY: // LDX Zero Page,Y
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddY(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                X = this->readMemory(cycles, addr);
                ldxSetFlags();
                break;
            }
            case INS_LDX_ABS: // LDX Absolute
            {
                uint16_t addr = fetch16(cycles);
                X = this->readMemory(cycles, addr);
                ldxSetFlags();
                break;
            }
            case INS_LDX_ABSY: // LDX Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                X = this->readMemory(cycles, addr);
                ldxSetFlags();
                break;
            }
            case INS_LDY_IMM: // LDY Immediate
            {
                uint8_t value = fetch(cycles);
                Y = value;
                ldySetFlags();
                break;
            }
            case INS_LDY_ZP: // LDY Zero Page
            {
                uint8_t addr = fetch(cycles);
                Y = this->readMemory(cycles, addr);
                ldySetFlags();
                break;
            }
            case INS_LDY_ZPX: // LDY Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                Y = this->readMemory(cycles, addr);
                ldySetFlags();
                break;
            }
            case INS_LDY_ABS: // LDY Absolute
            {
                uint16_t addr = fetch16(cycles);
                Y = this->readMemory(cycles, addr);
                ldySetFlags();
                break;
            }
            case INS_LDY_ABSX: // LDY Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                Y = this->readMemory(cycles, addr);
                ldySetFlags();
                break;
            }
            case INS_LSR_ACC: // LSR Accumulator
            {
                lsrSetFlags(cycles);
                break;
            }
            case INS_LSR_ZP: // LSR Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                lsrSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_LSR_ZPX: // LSR Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                lsrSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_LSR_ABS: // LSR Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                lsrSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_LSR_ABSX: // LSR Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                lsrSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_NOP: // NOP (No Operation)
            {
                // Do nothing, just decrement cycles
                --cycles;
                break;
            }
            case INS_ORA_IMM: // ORA Immediate
            {
                uint8_t value = fetch(cycles);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_ZP: // ORA Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_ZPX: // ORA Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_ABS: // ORA Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_ABSX: // ORA Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_ABSY: // ORA Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_INDX: // ORA (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_ORA_INDY: // ORA (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A |= value;
                oraSetFlags();
                break;
            }
            case INS_PHA: // PHA (Push Accumulator)
            {
                writeMemory(cycles, SP--, A);
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_PHP: // PHP (Push Processor Status)
            {
                uint8_t status = (N << 7) | (V << 6) | (1 << 5) | (B << 4) | (D << 3) | (I << 2) | (Z << 1) | C;
                writeMemory(cycles, SP--, status);
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_PLA: // PLA (Pull Accumulator)
            {
                A = readMemory(cycles, ++SP);
                --cycles; // Decrement cycles for the operation
                ldaSetFlags();
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_PLP: // PLP (Pull Processor Status)
            {
                uint8_t status = readMemory(cycles, ++SP);
                --cycles; // Decrement cycles for the operation
                plpSetFlags(status);
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_ROL_ACC: // ROL Accumulator
            {
                rolSetFlags(cycles);
                break;
            }
            case INS_ROL_ZP: // ROL Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rolSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROL_ZPX: // ROL Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rolSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROL_ABS: // ROL Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rolSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROL_ABSX: // ROL Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rolSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROR_ACC: // ROR Accumulator
            {
                rorSetFlags(cycles);
                break;
            }
            case INS_ROR_ZP: // ROR Zero Page   
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rorSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROR_ZPX: // ROR Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rorSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROR_ABS: // ROR Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rorSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_ROR_ABSX: // ROR Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                A = value;
                rorSetFlags(cycles);
                writeMemory(cycles, addr, A); // Write back to memory
                break;
            }
            case INS_RTI: // RTI (Return from Interrupt)
            {
                rtiSetFlags(cycles);
                uint8_t lowByte = readMemory(cycles, ++SP);
                uint8_t highByte = readMemory(cycles, ++SP);
                PC = (highByte << 8) | lowByte;
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_RTS: // RTS (Return from Subroutine)
            {
                uint8_t lowByte = readMemory(cycles, ++SP);
                uint8_t highByte = readMemory(cycles, ++SP);
                PC = ((highByte << 8) | lowByte) + 1; // Return to the instruction after JSR
                --cycles; // Decrement cycles for the operation
                --cycles; // Decrement cycles for the operation
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_SBC_IMM: // SBC Immediate
            {
                uint8_t value = fetch(cycles);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_ZP: // SBC Zero Page
            {
                uint8_t addr = fetch(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_ZPX: // SBC Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_ABS: // SBC Absolute
            {
                uint16_t addr = fetch16(cycles);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_ABSX: // SBC Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_ABSY: // SBC Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_INDX: // SBC (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SBC_INDY: // SBC (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                uint8_t value = this->readMemory(cycles, addr);
                uint16_t result = A - value - (1 - C);
                A = static_cast<uint8_t>(result & 0xFF);
                sbcSetFlags(value, result);
                break;
            }
            case INS_SEC: // SEC (Set Carry Flag)
            {
                C = 1; // Set Carry flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_SED: // SED (Set Decimal Flag)
            {
                D = 1; // Set Decimal flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_SEI: // SEI (Set Interrupt Disable)
            {
                I = 1; // Set Interrupt Disable flag
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_STA_ZP: // STA Zero Page
            {
                uint8_t addr = fetch(cycles);
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STA_ZPX: // STA Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STA_ABS: // STA Absolute
            {
                uint16_t addr = fetch16(cycles);
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STA_ABSX: // STA Absolute,X
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddX(cycles, addr);
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STA_ABSY: // STA Absolute,Y
            {
                uint16_t addr = fetch16(cycles);
                this->zeroPageAddY(cycles, addr);
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STA_INDX: // STA (Indirect,X)
            {
                uint16_t zpAddr = fetch(cycles);
                this->zeroPageAddX(cycles, zpAddr);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STA_INDY: // STA (Indirect),Y
            {
                uint8_t zpAddr = fetch(cycles);
                uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
                this->zeroPageAddY(cycles, addr);
                writeMemory(cycles, addr, A);
                break;
            }
            case INS_STX_ZP: // STX Zero Page
            {
                uint8_t addr = fetch(cycles);
                writeMemory(cycles, addr, X);
                break;
            }
            case INS_STX_ZPY: // STX Zero Page,Y
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddY(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                writeMemory(cycles, addr, X);
                break;
            }
            case INS_STX_ABS: // STX Absolute
            {
                uint16_t addr = fetch16(cycles);
                writeMemory(cycles, addr, X);
                break;
            }
            case INS_STY_ZP: // STY Zero Page
            {
                uint8_t addr = fetch(cycles);
                writeMemory(cycles, addr, Y);
                break;
            }
            case INS_STY_ZPX: // STY Zero Page,X
            {
                uint16_t addr = fetch(cycles);
                this->zeroPageAddX(cycles, addr);
                addr &= 0xFF; // Wrap around for zero page
                writeMemory(cycles, addr, Y);
                break;
            }
            case INS_STY_ABS: // STY Absolute
            {
                uint16_t addr = fetch16(cycles);
                writeMemory(cycles, addr, Y);
                break;
            }
            case INS_TAX: // TAX (Transfer Accumulator to X)
            {
                X = A;
                --cycles; // Decrement cycles for the operation
                taxSetFlags();
                break;
            }
            case INS_TAY: // TAY (Transfer Accumulator to Y)
            {
                Y = A;
                --cycles; // Decrement cycles for the operation
                taySetFlags();
                break;
            }
            case INS_TSX: // TSX (Transfer Stack Pointer to X)
            {
                X = SP;
                --cycles; // Decrement cycles for the operation
                tsxSetFlags();
                break;
            }
            case INS_TXA: // TXA (Transfer X to Accumulator)
            {
                A = X;
                --cycles; // Decrement cycles for the operation
                txaSetFlags();
                break;
            }
            case INS_TXS: // TXS (Transfer X to Stack Pointer)
            {
                SP = X;
                --cycles; // Decrement cycles for the operation
                break;
            }
            case INS_TYA: // TYA (Transfer Y to Accumulator)
            {
                A = Y;
                --cycles; // Decrement cycles for the operation
                tyaSetFlags();
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