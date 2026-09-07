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
        case 0xA9: { // LDA Immediate
            uint8_t value = memory[PC];
            A = value;
            PC++;
            break;
        }
        // Add more opcodes as needed
        default:
            // Handle unknown opcode
            break;
    }
}