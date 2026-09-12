#include "CPU6507.hh"

CPU6507::CPU6507() : A(0), X(0), Y(0), SP(0), P(0x24), PC(0x1FFC) {
    for (auto& byte : memory) {
        byte = 0;
    }
    bus = new Atari2600Bus(this);
}

CPU6507::~CPU6507() {
    delete bus;
}

void CPU6507::setRenderer(SDL_Renderer* renderer) {
    bus->setRenderer(renderer);
}

void CPU6507::loadProgram(const std::string& filename) {
    bus->loadCartridge(filename);
}

void CPU6507::tick() {
    bus->tick();
}

void CPU6507::reset() {
    A = 0;
    X = 0;
    Y = 0;
    SP = 0; // Stack pointer initialized to the top of the stack
    P = 0x24;
    PC = 0x1FFC;

    for (auto& byte : memory) {
        byte = 0;
    }
}

void CPU6507::setFlagZ(uint8_t value) {
    if (value == 0) {
        this->SR |= 0x02; // Set the zero flag
    } else {
        this->SR &= ~0x80; // Clear the negative flag
    }
}

void CPU6507::setFlagN(uint8_t value) {
    if (value & 0x80) {
        this->SR |= 0x80; // Set the negative flag
    } else {
        this->SR &= ~0x80; // Clear the negative flag
    }
}

void CPU6507::setFlagC(uint8_t value) {
    if (value) {
        this->SR |= 0x01; // Set the carry flag
    } else {
        this->SR &= ~0x01; // Clear the carry flag
    }
}

void CPU6507::setFlagV(uint8_t value) {
    if (value) {
        this->SR |= 0x40; // Set the overflow flag
    } else {
        this->SR &= ~0x40; // Clear the overflow flag
    }
}

void CPU6507::setFlagD(uint8_t value) {
    if (value) {
        this->SR |= 0x08; // Set the decimal mode flag
    } else {
        this->SR &= ~0x08; // Clear the decimal mode flag
    }
}

void CPU6507::setFlagI(uint8_t value) {
    if (value) {
        this->SR |= 0x04; // Set the interrupt disable flag
    } else {
        this->SR &= ~0x04; // Clear the interrupt disable flag
    }
}

void CPU6507::setFlagB(uint8_t value) {
    if (value) {
        this->SR |= 0x10; // Set the break command flag
    } else {
        this->SR &= ~0x10; // Clear the break command flag
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
            setFlagZ(this->Y); // Set the zero flag based on the value of Y
            setFlagN(this->Y); // Set the negative flag based on the value of Y
            break;
        }
        case 0xAA: { // TAX
            this->X = this->A; // Transfer the value of A into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0xBA: { // TSX
            this->X = this->SP; // Transfer the value of SP into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0x98: { // TYA
            this->A = this->Y; // Transfer the value of Y into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x8A: { // TXA
            this->A = this->X; // Transfer the value of X into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x9A: { // TXS
            this->SP = this->X; // Transfer the value of X into SP
            break;
        }
        case 0xA9: { // LDA Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A = value; // Load the immediate value into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xA2: { // LDX Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->X = value; // Load the immediate value into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0xA0: { // LDY Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->Y = value; // Load the immediate value into Y
            setFlagZ(this->Y); // Set the zero flag based on the value of Y
            setFlagN(this->Y); // Set the negative flag based on the value of Y
            break;
        }
        case 0xA5: { // LDA Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->A = this->bus->read(address); // Load the value from the zero page address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xB5: { // LDA Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->A = this->bus->read(address); // Load the value from the zero page address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xAD: { // LDA Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->A = this->bus->read(address); // Load the value from the absolute address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xBD: { // LDA Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->A = this->bus->read(address); // Load the value from the absolute address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xB9: { // LDA Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->A = this->bus->read(address); // Load the value from the absolute address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xA1: { // LDA (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            this->A = this->bus->read(effective_address); // Load the value from the effective address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xB1: { // LDA (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            this->A = this->bus->read(effective_address); // Load the value from the effective address into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xA6: { // LDX Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->X = this->bus->read(address); // Load the value from the zero page address into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0xB6: { // LDX Zero Page,Y
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->X = this->bus->read(address); // Load the value from the zero page address into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0xAE: { // LDX Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->X = this->bus->read(address); // Load the value from the absolute address into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0xBE: { // LDX Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->X = this->bus->read(address); // Load the value from the absolute address into X
            setFlagZ(this->X); // Set the zero flag based on the value of X
            setFlagN(this->X); // Set the negative flag based on the value of X
            break;
        }
        case 0xA4: { // LDY Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->Y = this->bus->read(address); // Load the value from the zero page address into Y
            setFlagZ(this->Y); // Set the zero flag based on the value of Y
            setFlagN(this->Y); // Set the negative flag based on the value of Y
            break;
        }
        case 0xB4: { // LDY Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->Y = this->bus->read(address); // Load the value from the zero page address into Y
            setFlagZ(this->Y); // Set the zero flag based on the value of Y
            setFlagN(this->Y); // Set the negative flag based on the value of Y
            break;
        }
        case 0xAC: { // LDY Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->Y = this->bus->read(address); // Load the value from the absolute address into Y
            setFlagZ(this->Y); // Set the zero flag based on the value of Y
            setFlagN(this->Y); // Set the negative flag based on the value of Y
            break;
        }
        case 0xBC: { // LDY Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->Y = this->bus->read(address); // Load the value from the absolute address into Y
            setFlagZ(this->Y); // Set the zero flag based on the value of Y
            setFlagN(this->Y); // Set the negative flag based on the value of Y
            break;
        }
        case 0x85: { // STA Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->bus->write(address, this->A); // Store the value of A into the zero page address
            break;
        }
        case 0x95: { // STA Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->bus->write(address, this->A); // Store the value of A into the zero page address
            break;
        }
        case 0x8D: { // STA Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->bus->write(address, this->A); // Store the value of A into the absolute address
            break;
        }
        case 0x9D: { // STA Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->bus->write(address, this->A); // Store the value of A into the absolute address
            break;
        }
        case 0x99: { // STA Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->bus->write(address, this->A); // Store the value of A into the absolute address
            break;
        }
        case 0x81: { // STA (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            this->bus->write(effective_address, this->A); // Store the value of A into the effective address
            break;
        }
        case 0x91: { // STA (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            this->bus->write(effective_address, this->A); // Store the value of A into the effective address
            break;
        }
        case 0x86: { // STX Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->bus->write(address, this->X); // Store the value of X into the zero page address
            break;
        }
        case 0x96: { // STX Zero Page,Y
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->bus->write(address, this->X); // Store the value of X into the zero page address
            break;
        }
        case 0x8E: { // STX Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->bus->write(address, this->X); // Store the value of X into the absolute address
            break;
        }
        case 0x84: { // STY Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->bus->write(address, this->Y); // Store the value of Y into the zero page address
            break;
        }
        case 0x94: { // STY Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->bus->write(address, this->Y); // Store the value of Y into the zero page address
            break;
        }
        case 0x8C: { // STY Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->bus->write(address, this->Y); // Store the value of Y into the absolute address
            break;
        }
        case 0x48: { // PHA
            memory[SP] = this->A; // Push the value of A onto the stack
            SP--; // Decrement the stack pointer
            break;
        }
        case 0x08: { // PHP
            memory[SP] = this->SR; // Push the status register onto the stack
            SP--; // Decrement the stack pointer
            break;
        }
        case 0x68: { // PLA
            SP++; // Increment the stack pointer
            this->A = memory[SP]; // Pull the value from the stack into A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x28: { // PLP
            SP++; // Increment the stack pointer
            this->SR = memory[SP]; // Pull the value from the stack into the status register
            setFlagZ(this->SR); // Set the zero flag based on the value of SR
            setFlagN(this->SR); // Set the negative flag based on the value of SR
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            setFlagV(this->SR); // Set the overflow flag based on the value of SR
            setFlagD(this->SR); // Set the decimal mode flag based on the value of
            setFlagI(this->SR); // Set the interrupt disable flag based on the value of SR
            break;
        }
        case 0x69: { // ADC Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x65: { // ADC Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x75: { // ADC Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x6D: { // ADC Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x7D: { // ADC Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x79: { // ADC Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x61: { // ADC (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x71: { // ADC (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            uint16_t result = this->A + value + (this->SR & 0x01); // Add A, value, and carry flag
            this->SR = (result > 0xFF) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xE9: { // SBC Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xE5: { // SBC Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xF5: { // SBC Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xED: { // SBC Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xFD: { // SBC Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xF9: { // SBC Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xE1: { // SBC (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xF1: { // SBC (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            uint16_t result = this->A - value - (1 - (this->SR & 0x01)); // Subtract value and borrow from A
            this->SR = (result < 0x100) ? (this->SR | 0x01) : (this->SR & ~0x01); // Set carry flag
            this->A = static_cast<uint8_t>(result); // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            setFlagV(((this->A ^ result) & (value ^ result) & 0x80) != 0); // Set overflow flag
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x29: { // AND Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= value; // Perform bitwise AND with A and the immediate value
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x25: { // AND Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x35: { // AND Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x2D: { // AND Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x3D: { // AND Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x39: { // AND Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x21: { // AND (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x31: { // AND (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            this->A &= value; // Perform bitwise AND with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x49: { // EOR Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A ^= value; // Perform bitwise EOR with A and the immediate value
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x45: { // EOR Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x55: { // EOR Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x4D: { // EOR Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x5D: { // EOR Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x59: { // EOR Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x41: { // EOR (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x51: { // EOR (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            this->A ^= value; // Perform bitwise EOR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x09: { // ORA Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A |= value; // Perform bitwise OR with A and the immediate value
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x05: { // ORA Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x15: { // ORA Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x0D: { // ORA Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x1D: { // ORA Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x19: { // ORA Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x01: { // ORA (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address + this->X); // Calculate effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0x11: { // ORA (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            this->A |= value; // Perform bitwise OR with A and the value from memory
            setFlagZ(this->A); // Set the zero flag based on the value of A
            setFlagN(this->A); // Set the negative flag based on the value of A
            break;
        }
        case 0xC9: { // CMP Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            uint16_t result = this->A - value; // Subtract the immediate value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xC5: { // CMP Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xD5: { // CMP Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xCD: { // CMP Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xDD: { // CMP Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xD9: { // CMP Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xC1: { // CMP (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Calculate effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xD1: { // CMP (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address); // Fetch the effective address from zero page
            effective_address += this->Y; // Add the value of Y to the effective address
            uint8_t value = this->bus->read(effective_address); // Fetch the value from the effective address
            uint16_t result = this->A - value; // Subtract the value from A
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xE0: { // CPX Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            uint16_t result = this->X - value; // Subtract the immediate value from X
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of
            break;
        }
        case 0xE4: { // CPX Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->X - value; // Subtract the value from X
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xEC: { // CPX Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->X - value; // Subtract the value from X
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of
            break;
        }
        case 0xC0: { // CPY Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            uint16_t result = this->Y - value; // Subtract the immediate value from Y
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xC4: { // CPY Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            uint16_t result = this->Y - value; // Subtract the value from Y
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xCC: { // CPY Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            uint16_t result = this->Y - value; // Subtract the value from Y
            setFlagZ(result & 0xFF); // Set the zero flag based on the result
            setFlagN(result & 0xFF); // Set the negative flag based on the result
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x24: { // BIT Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            this->SR = (this->SR & ~0xC0) | (value & 0xC0); // Set the negative and overflow flags based on the value
            this->SR |= (this->A & value) ? 0 : 0x02; // Set the zero flag based on the result of A AND value
            break;
        }
        case 0x2C: { // BIT Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            this->SR = (this->SR & ~0xC0) | (value & 0xC0); // Set the negative and overflow flags based on the value
            this->SR |= (this->A & value) ? 0 : 0x02; // Set the zero flag based on the result of A AND value
            break;
        }
        case 0xE6: { // INC Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            value++; // Increment the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xF6: { // INC Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            value++; // Increment the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xEE: { // INC Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            value++; // Increment the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xFE: { // INC Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            value++; // Increment the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xE8: { // INX
            this->X++; // Increment the X register
            setFlagZ(this->X); // Set the zero flag based on the new value of X
            setFlagN(this->X); // Set the negative flag based on the new value of X
            break;
        }
        case 0xC8: { // INY
            this->Y++; // Increment the Y register
            setFlagZ(this->Y); // Set the zero flag based on the new value of Y
            setFlagN(this->Y); // Set the negative flag based on the new value of Y
            break;
        }
        case 0xC6: { // DEC Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            value--; // Decrement the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xD6: { // DEC Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the zero page address
            value--; // Decrement the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xCE: { // DEC Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            value--; // Decrement the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xDE: { // DEC Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t value = this->bus->read(address); // Fetch the value from the absolute address
            value--; // Decrement the value
            this->bus->write(address, value); // Write the new value back to memory
            setFlagZ(value); // Set the zero flag based on the new value
            setFlagN(value); // Set the negative flag based on the new value
            break;
        }
        case 0xCA: { // DEX
            this->X--; // Decrement the X register
            setFlagZ(this->X); // Set the zero flag based on the new value of X
            setFlagN(this->X); // Set the negative flag based on the new value of
            break;
        }
        case 0x88: { // DEY
            this->Y--; // Decrement the Y register
            setFlagZ(this->Y); // Set the zero flag based on the new value of Y
            setFlagN(this->Y); // Set the negative flag based on the new value of Y
            break;
        }
        case 0x0A: { // ASL Accumulator
            this->SR = (this->SR & ~0x01) | ((this->A >> 7) & 0x01); // Set the carry flag based on the old bit 7 of A
            this->A <<= 1; // Shift A left by one
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x06: { // ASL Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, this->bus->read(address) << 1); // Shift the value at the zero page address left by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x16: { // ASL Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, this->bus->read(address) << 1); // Shift the value at the zero page address left by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x0E: { // ASL Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, this->bus->read(address) << 1); // Shift the value at the absolute address left by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x1E: { // ASL Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, this->bus->read(address) << 1); // Shift the value at the absolute address left by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x4A: { // LSR Accumulator
            this->SR = (this->SR & ~0x01) | (this->A & 0x01); // Set the carry flag based on the old bit 0 of A
            this->A >>= 1; // Shift A right by one
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(0x79); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x46: { // LSR Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->SR = (this->SR & ~0x01) | (this->bus->read(address & 0x01)); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, this->bus->read(address) >> 1); // Shift the value at the zero page address right by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(0x79); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x56: { // LSR Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, this->bus->read(address) >> 1); // Shift the value at the zero page address right by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(0x79); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x4E: { // LSR Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, this->bus->read(address) >> 1); // Shift the value at the absolute address right by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(0x79); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x5E: { // LSR Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, this->bus->read(address) >> 1); // Shift the value at the absolute address right by one
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(0x79); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x2A: { // ROL Accumulator
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | ((this->A >> 7) & 0x01); // Set the carry flag based on the old bit 7 of A
            this->A = (this->A << 1) | old_carry; // Shift A left by one and add the old carry
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x26: { // ROL Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, (this->bus->read(address) << 1) | old_carry); // Shift the value at the zero page address left by one and add the old carry
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x36: { // ROL Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, (this->bus->read(address) << 1) | old_carry); // Shift the value at the zero page address left by one and add the old carry
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x2E: { // ROL Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, (this->bus->read(address) << 1) | old_carry); // Shift the value at the absolute address left by one and add the old carry
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x3E: { // ROL Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | ((this->bus->read(address) >> 7) & 0x01); // Set the carry flag based on the old bit 7 of memory[address]
            this->bus->write(address, (this->bus->read(address) << 1) | old_carry); // Shift the value at the absolute address left by one and add the old carry
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x6A: { // ROR Accumulator
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | (this->A & 0x01); // Set the carry flag based on the old bit 0 of A
            this->A = (this->A >> 1) | (old_carry << 7); // Shift A right by one and add the old carry to bit 7
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of
            break;
        }
        case 0x66: { // ROR Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, (this->bus->read(address) >> 1) | (old_carry << 7)); // Shift the value at the zero page address right by one and add the old carry to bit 7
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x76: { // ROR Zero Page,X
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, (this->bus->read(address) >> 1) | (old_carry << 7)); // Shift the value at the zero page address right by one and add the old carry to bit 7
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x6E: { // ROR Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, (this->bus->read(address) >> 1) | (old_carry << 7)); // Shift the value at the absolute address right by one and add the old carry to bit 7
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x7E: { // ROR Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            uint8_t old_carry = this->SR & 0x01; // Store the old carry flag
            this->SR = (this->SR & ~0x01) | (this->bus->read(address) & 0x01); // Set the carry flag based on the old bit 0 of memory[address]
            this->bus->write(address, (this->bus->read(address) >> 1) | (old_carry << 7)); // Shift the value at the absolute address right by one and add the old carry to bit 7
            setFlagZ(this->bus->read(address)); // Set the zero flag based on the new value
            setFlagN(this->bus->read(address)); // Set the negative flag based on the new value
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x4C: { // JMP Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC = address; // Set PC to the new address
            break;
        }
        case 0x6C: { // JMP Indirect
            uint16_t pointer = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the pointer address
            uint16_t address = this->bus->read(pointer) | (this->bus->read((pointer & 0xFF00) | ((pointer + 1) & 0x00FF)) << 8); // Fetch the effective address from the pointer
            PC = address; // Set PC to the new address
            break;
        }
        case 0x20: { // JSR Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->bus->write(--this->SP, (PC >> 8) & 0xFF); // Push the high byte of PC onto the stack
            this->bus->write(--this->SP, PC & 0xFF); // Push the low byte of PC onto the stack
            PC = address; // Set PC to the new address
            break;
        }
        case 0x40: { // RTI
            this->SR = this->bus->read(++this->SP); // Pull the status register from the stack
            uint8_t low_byte = this->bus->read(++this->SP); // Pull the low byte of PC from the stack
            uint8_t high_byte = this->bus->read(++this->SP); // Pull the high byte of PC from the stack
            PC = (high_byte << 8) | low_byte; // Set PC to the new address
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            setFlagZ(this->SR); // Set the zero flag based on the value of SR
            setFlagN(this->SR); // Set the negative flag based on the value of SR
            setFlagV(this->SR); // Set the overflow flag based on the value of SR
            setFlagI(this->SR); // Set the interrupt disable flag based on the value of SR
            setFlagD(this->SR); // Set the decimal mode flag based on the value of SR
            break;
        }
        case 0x60: { // RTS
            uint8_t low_byte =this->bus->read(++this->SP); // Pull the low byte of PC from the stack
            uint8_t high_byte = this->bus->read(++this->SP); // Pull the high byte of PC from the stack
            PC = (high_byte << 8) | low_byte; // Set PC to the new address
            PC++; // Increment PC to point to the next instruction after the JSR
            break;
        }
        case 0x10: { // BPL (Branch if Positive)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x80) == 0) { // Check if the negative flag is clear
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0x30: { // BMI (Branch if Negative)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x80) != 0) { // Check if the negative flag is set
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0x50: { // BVC (Branch if Overflow Clear)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x40) == 0) { // Check if the overflow flag is clear
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0x70: { // BVS (Branch if Overflow Set)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x40) != 0) { // Check if the overflow flag is set
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0x90: { // BCC (Branch if Carry Clear)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x01) == 0) { // Check if the carry flag is clear
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0xB0: { // BCS (Branch if Carry Set)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x01) != 0) { // Check if the carry flag is set
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0xD0: { // BNE (Branch if Not Equal)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x02) == 0) { // Check if the zero flag is clear
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0xF0: { // BEQ (Branch if Equal)
            int8_t offset = static_cast<int8_t>(this->bus->read(PC)); // Fetch the signed offset
            PC++; // Increment PC to point to the next instruction
            if ((this->SR & 0x02) != 0) { // Check if the zero flag is set
                PC += offset; // Branch to the new address
            }
            break;
        }
        case 0x00: { // BRK (Force Interrupt)
            PC++; // Increment PC to point to the next instruction
            this->bus->write(--this->SP, (PC >> 8) & 0xFF); // Push the high byte of PC onto the stack
            this->bus->write(--this->SP, PC & 0xFF); // Push the low byte of PC onto the stack
            this->bus->write(--this->SP, this->SR | 0x10); // Push the status register onto the stack with the
            this->SR |= 0x04; // Set the interrupt disable flag
            uint16_t interrupt_vector = memory[0xFFFE] | (memory[0xFFFF] << 8); // Fetch the interrupt vector address
            PC = interrupt_vector; // Set PC to the interrupt vector address
            break;
        }
        case 0x18: { // CLC (Clear Carry Flag)
            this->SR &= ~0x01; // Clear the carry flag
            break;
        }
        case 0x58: { // CLI (Clear Interrupt Disable)
            this->SR &= ~0x04; // Clear the interrupt disable flag
            break;
        }
        case 0xD8: { // CLD (Clear Decimal Mode)
            this->SR &= ~0x08; // Clear the decimal mode flag
            break;
        }
        case 0xB8: { // CLV (Clear Overflow Flag)
            this->SR &= ~0x40; // Clear the overflow flag
            break;
        }
        case 0x38: { // SEC (Set Carry Flag)
            this->SR |= 0x01; // Set the carry flag
            break;
        }
        case 0x78: { // SEI (Set Interrupt Disable)
            this->SR |= 0x04; // Set the interrupt disable flag
            break;
        }
        case 0xF8: { // SED (Set Decimal Mode)
            this->SR |= 0x08; // Set the decimal mode flag
            break;
        }
        case 0xEA: { // NOP (No Operation)
            // Do nothing
            break;
        }
        case 0x87: { // SAX Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->bus->write(address, this->A & this->X); // Store A AND X into the zero page address
            break;
        }
        case 0x97: { // SAX Zero Page,Y
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->bus->write(address, this->A & this->X); // Store A AND X into the zero page address
            break;
        }
        case 0x8F: { // SAX Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->bus->write(address, this->A & this->X); // Store A AND X into the absolute address
            break;
        }
        case 0x83: { // SAX (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Fetch the effective address from zero page with X offset
            this->bus->write(effective_address, this->A & this->X); // Store A AND X into the effective address
            break;
        }
        case 0xA7: { // LAX Zero Page
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            this->A = this->bus->read(address); // Load the value from the zero page address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of
            break;
        }
        case 0xB7: { // LAX Zero Page,Y
            uint8_t address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->A = this->bus->read(address); // Load the value from the zero page address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0xAF: { // LAX Absolute
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            this->A = this->bus->read(address); // Load the value from the absolute address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0xA3: { // LAX (Indirect,X)
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read((zp_address + this->X) & 0xFF); // Fetch the effective address from zero page with X offset
            this->A = this->bus->read(effective_address); // Load the value from the effective address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0xB3: { // LAX (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address) + this->Y; // Fetch the effective address from zero page with Y offset
            this->A = this->bus->read(effective_address); // Load the value from the effective address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0x0B: { // ANC Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= value; // AND the value with A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x2B: { // ANC Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= value; // AND the value with A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x4B: { // ALR Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= value; // AND the value with A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0x6B: { // ARR Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= value; // AND the value with A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            setFlagV(this->SR); // Set the overflow flag based on the value of SR
            break;
        }
        case 0xCB: { // AXS Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= this->X; // AND A with X
            this->A -= value; // Subtract the immediate value from A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            break;
        }
        case 0xEB: { // SBC Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            uint16_t result = this->A - value - (this->SR & 0x01); // Subtract the value and the carry flag from A
            this->SR = (this->SR & ~0x01) | ((result >> 8) & 0x01); // Set the carry flag based on the result
            this->A = result & 0xFF; // Store the result in A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            setFlagC(this->SR); // Set the carry flag based on the value of SR
            setFlagV(this->SR); // Set the overflow flag based on the value of SR
            break;
        }
        case 0xBB: { // LAS Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->A = this->bus->read(address); // Load the value from the absolute address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0x8B: { // XAA Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A &= this->X; // AND A with X
            this->A &= value; // AND the result with the immediate value
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of
            break;
        }
        case 0xAB: { // OAL Immediate
            uint8_t value = this->bus->read(PC); // Fetch the immediate value
            PC++; // Increment PC to point to the next instruction
            this->A |= value; // OR the immediate value with A
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0xBF: { // LAX Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->A = this->bus->read(address); // Load the value from the absolute address into A
            this->X = this->A; // Load the value into X as well
            setFlagZ(this->A); // Set the zero flag based on the new value of A
            setFlagN(this->A); // Set the negative flag based on the new value of A
            break;
        }
        case 0x93: { // LAX (Indirect),Y
            uint8_t zp_address = this->bus->read(PC); // Fetch the zero page address
            PC++; // Increment PC to point to the next instruction
            uint8_t effective_address = this->bus->read(zp_address) + this->Y; // Fetch the effective address from zero page with Y offset
            this->A = this->bus->read(effective_address); // Load the value from the effective address into A
            this->X = this->A; // Load the value into X as well
            break;
        }
        case 0x9F: { // SAX Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->bus->write(address, this->A & this->X); // Store A AND X into the absolute address
            break;
        }
        case 0x9C: { // SHY Absolute,X
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->X; // Add the value of X to the address
            this->bus->write(address, this->Y & (address >> 8)); // Store Y AND high byte of address into the absolute address
            break;
        }
        case 0x9E: { // SHX Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->bus->write(address, this->X & (address >> 8)); // Store X AND high byte of address into the absolute address
            break;
        }
        case 0x9B: { // TAS Absolute,Y
            uint16_t address = this->bus->read(PC) | (this->bus->read(PC + 1) << 8); // Fetch the absolute address
            PC += 2; // Increment PC to point to the next instruction
            address += this->Y; // Add the value of Y to the address
            this->bus->write(address, this->A & this->X & (address >> 8)); // Store A AND X AND high byte of address into the absolute address
            break;
        }
        default:
            // Handle unknown opcode
            break;
    }
}