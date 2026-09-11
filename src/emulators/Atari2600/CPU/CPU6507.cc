#include "CPU6507.hh"

CPU6507::CPU6507() : A(0), X(0), Y(0), SP(0x0100), P(0x24), PC(0x1FFC) {
    for (auto& byte : memory) {
        byte = 0;
    }
}

void CPU6507::reset() {
    A = 0;
    X = 0;
    Y = 0;
    SP = 0x0100; // Stack pointer initialized to the top of the stack
    P = 0x24;
    PC = 0x1FFC;

    for (auto& byte : memory) {
        byte = 0;
    }
}

void CPU6507::step() {
    // Fetch the opcode at the current PC
    uint8_t opcode = memory[PC];
    PC++; // Increment PC to point to the next instruction

    // Decode and execute the opcode
    switch (opcode) {
        case 0xA8: { // LDA Immediate
            this->Y = this->A; // Load the value of A into Y
            this->SR |= 3; // Set the zero and negative flags based on the value of A
            break;
        }
        case 0xAA: { // TAX
            this->X = this->A; // Transfer the value of A into X
            this->SR |= 3; // Set the zero and negative flags based on the value of X
            break;
        }
        case 0xBA: { // TSX
            this->X = this->SP; // Transfer the value of SP into X
            this->SR |= 3; // Set the zero and negative flags based on the value of X
            break;
        }
        case 0x98: { // TYA
            this->A = this->Y; // Transfer the value of Y into A
            this->SR |= 3; // Set the zero and negative flags based on the value of A
            break;
        }
        case 0x8A: { // TXA
            this->A = this->X; // Transfer the value of X into A
            this->SR |= 3; // Set the zero and negative flags based on the value of A
            break;
        }
        case 0x9A: { // TXS
            this->SP = this->X; // Transfer the value of X into SP
            break;
        }
        case 0xA9: { // LDA Immediate
            uint8_t value = memory[PC]; // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A = value; // Load the immediate value into A
            this->SR |= 3; // Set the zero and negative flags based on the value of A
            break;
        }
        case 0xA2: { // LDX Immediate
            uint8_t value = memory[PC]; // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->X = value; // Load the immediate value into X
            this->SR |= 3; // Set the zero and negative flags based on the value of X
            break;
        }
        case 0xA0: { // LDY Immediate
            uint8_t value = memory[PC]; // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->Y = value; // Load the immediate value into Y
            this->SR |= 3; // Set the zero and negative flags based on the value of Y
            break;
        }
        // Add more opcodes as neede
        default:
            // Handle unknown opcode
            break;
    }
}