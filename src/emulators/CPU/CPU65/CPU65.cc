#include "CPU65.hh"
#include "opcodes.hh"

#include <iostream>

CPU65::CPU65(Bus* bus) : bus(bus) {
    reset();
}

void CPU65::reset() {
    A = 0;
    X = 0;
    Y = 0;
    SP = 0xFD; // Stack Pointer starts at 0xFD after reset
    halted = false;
    const uint8_t lowByte = bus->readMemory(0xFFFC);
    const uint8_t highByte = bus->readMemory(0xFFFD);
    PC = static_cast<uint16_t>(lowByte) |
         (static_cast<uint16_t>(highByte) << 8);
    C = 0;
    Z = 0;
    I = 1; // Interrupts are disabled after reset
    D = 0;
    B = 0;
    V = 0;
    N = 0;
}

uint8_t CPU65::fetch(uint32_t &cycles) {
    uint8_t instruction = bus->readMemory(PC);
    PC++;
    cycles++;
    return instruction;
}

uint16_t CPU65::fetch16(uint32_t &cycles) {
    uint16_t lowByte = fetch(cycles);
    uint16_t highByte = fetch(cycles);
    return (highByte << 8) | lowByte;
}

uint8_t CPU65::readMemory(uint32_t &cycles, uint16_t address) {
    uint8_t data = bus->readMemory(address);
    cycles++;
    return data;
}

void CPU65::writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) {
    bus->writeMemory(address, value);
    cycles++;
}
void CPU65::push(uint32_t &cycles, uint8_t v) { 
    writeMemory(cycles, 0x0100 | SP, v); 
    --SP; 
}

uint8_t CPU65::pull(uint32_t &cycles) {
     ++SP; 
     return readMemory(cycles, 0x0100 | SP); 
}

void CPU65::zeroPageAddX(uint32_t &cycles, uint16_t &address) {
    address = (address + X);
    cycles++;
}

void CPU65::zeroPageAddY(uint32_t &cycles, uint16_t &address) {
    address = (address + Y);
    cycles++;
}

uint8_t CPU65::asl(uint8_t v) 
{ 
    C = v >> 7; 
    v <<= 1;
    Z = (v == 0); 
    N = v >> 7;
    return v; 
}

uint8_t CPU65::lsr(uint8_t v) 
{ 
    C = v & 1;  
    v >>= 1; 
    Z = (v == 0); 
    N = 0;      
    return v; 
}

uint8_t CPU65::rol(uint8_t v) 
{ 
    uint8_t c = C; 
    C = v >> 7; 
    v = (v << 1) | c;  
    Z = (v == 0); 
    N = v >> 7; 
    return v; 
}

uint8_t CPU65::ror(uint8_t v) 
{ 
    uint8_t c = C; 
    C = v & 1;  
    v = (v >> 1) | (c << 7); 
    Z = (v == 0); 
    N = v >> 7; 
    return v; 
}

uint16_t CPU65::absIndexed(uint32_t &cycles, uint8_t idx, bool write)
{
    uint16_t base = fetch16(cycles);

    uint16_t addr = base + idx;

    if (write || (base & 0xFF00) != (addr & 0xFF00))
        ++cycles;

    return addr;
}

void CPU65::adc(uint8_t v) {
    const uint16_t bin = A + v + C;
    if (!D) {
        V = (~(A ^ v) & (A ^ bin) & 0x80) != 0;
        C = bin > 0xFF;
        A = bin & 0xFF;
        Z = (A == 0); N = A >> 7;
        return;
    }
    int lo = (A & 0x0F) + (v & 0x0F) + C;
    if (lo > 9) lo += 6;
    int hi = (A >> 4) + (v >> 4) + (lo > 0x0F);
    Z = ((bin & 0xFF) == 0);
    N = (hi & 8) != 0;
    V = (~(A ^ v) & (A ^ (hi << 4)) & 0x80) != 0;
    if (hi > 9) hi += 6;
    C = hi > 0x0F;
    A = ((hi << 4) | (lo & 0x0F)) & 0xFF;
}

void CPU65::sbc(uint8_t v) {
    uint8_t C_in = C;
    const int bin = A - v - (1 - C);
    V = ((A ^ v) & (A ^ bin) & 0x80) != 0;
    C = bin >= 0;
    Z = ((bin & 0xFF) == 0);
    N = (bin >> 7) & 1;
    if (!D) { A = bin & 0xFF; return; }
    int lo = (A & 0x0F) - (v & 0x0F) - (1 - C_in);
    int hi = (A >> 4) - (v >> 4);
    if (lo < 0) { lo -= 6; hi--; }
    if (hi < 0) hi -= 6;
    A = ((hi << 4) | (lo & 0x0F)) & 0xFF;
}

void CPU65::branch(uint32_t &cycles, bool cond) {
      int8_t off = static_cast<int8_t>(fetch(cycles));
      if (cond) { 
        uint16_t old = PC; 
        PC += off; 
        ++cycles; 
        if ((old ^ PC) & 0xFF00)
            ++cycles; 
     }
}

uint8_t CPU65::getStatus(bool breakFlag) const
{
    return
        (static_cast<uint8_t>(N) << 7) |
        (static_cast<uint8_t>(V) << 6) |
        (1u << 5) |
        (static_cast<uint8_t>(breakFlag) << 4) |
        (static_cast<uint8_t>(D) << 3) |
        (static_cast<uint8_t>(I) << 2) |
        (static_cast<uint8_t>(Z) << 1) |
        static_cast<uint8_t>(C);
}

void CPU65::isb(uint16_t addr, uint32_t& cycles)
{
    // INC memory
    uint8_t value = readMemory(cycles, addr);
    ++value;
    writeMemory(cycles, addr, value);

    // SBC memory
    sbc(value);
}

void CPU65::slo(uint16_t addr, uint32_t& cycles)
{
    uint8_t value = readMemory(cycles, addr);

    // ASL memory
    value = asl(value);

    writeMemory(cycles, addr, value);

    // ORA
    A |= value;

    Z = (A == 0);
    N = (A & 0x80) != 0;
}

void CPU65::rla(uint16_t addr, uint32_t& cycles)
{
    uint8_t value = readMemory(cycles, addr);

    value = rol(value);

    writeMemory(cycles, addr, value);

    // AND
    A &= value;

    Z = (A == 0);
    N = (A & 0x80) != 0;
}

void CPU65::sre(uint16_t addr, uint32_t& cycles)
{
    uint8_t value = readMemory(cycles, addr);

    // LSR M
    value = lsr(value);

    writeMemory(cycles, addr, value);

    // EOR M
    A ^= value;

    Z = (A == 0);
    N = (A & 0x80) != 0;

    // V is unchanged
}

void CPU65::rra(uint16_t addr, uint32_t& cycles)
{
    uint8_t value = readMemory(cycles, addr);

    // ROR M
    value = ror(value);

    writeMemory(cycles, addr, value);

    // ADC M
    adc(value);
}

void CPU65::ahx(uint16_t addr, uint8_t high)
{
    uint8_t value =
        A & X & static_cast<uint8_t>(high + 1);

    bus->writeMemory(addr, value);
}

void CPU65::shy(uint16_t addr)
{
    uint8_t high = static_cast<uint8_t>(addr >> 8);

    uint8_t value =
        Y & static_cast<uint8_t>(high + 1);

    bus->writeMemory(addr, value);
}

void CPU65::shx(uint16_t addr)
{
    uint8_t high = static_cast<uint8_t>(addr >> 8);

    uint8_t value =
        X & static_cast<uint8_t>(high + 1);

    bus->writeMemory(addr, value);
}

void CPU65::tas(uint16_t addr)
{
    SP = A & X;

    uint8_t high = static_cast<uint8_t>(addr >> 8);

    uint8_t value =
        SP & static_cast<uint8_t>(high + 1);

    bus->writeMemory(addr, value);
}

void CPU65::xaa(uint8_t value)
{
    constexpr uint8_t MAGIC = 0xEE;

    A = X & value & MAGIC;

    Z = (A == 0);
    N = (A & 0x80) != 0;
}

void CPU65::dcp(uint16_t addr, uint32_t& cycles)
{
    uint8_t value = readMemory(cycles, addr);

    --value;

    writeMemory(cycles, addr, value);

    cmpSetFlags(value);
}

void CPU65::alr(uint8_t value)
{
    A &= value;

    C = (A & 0x01) != 0;

    A >>= 1;

    Z = (A == 0);
    N = false;
}

void CPU65::arr(uint8_t value)
{
    A &= value;

    bool oldCarry = C;

    C = (A & 0x01) != 0;

    A >>= 1;

    if (oldCarry)
        A |= 0x80;

    Z = (A == 0);
    N = (A & 0x80) != 0;

    C = (A & 0x40) != 0;
    V = ((A & 0x40) != 0) ^ ((A & 0x20) != 0);
}

void CPU65::axs(uint8_t value)
{
    uint8_t ax = A & X;

    uint16_t result =
        static_cast<uint16_t>(ax) -
        static_cast<uint16_t>(value);

    X = static_cast<uint8_t>(result);

    C = ax >= value;
    Z = (X == 0);
    N = (X & 0x80) != 0;
}

void CPU65::las(uint16_t addr, uint32_t& cycles)
{
    uint8_t value = readMemory(cycles, addr);

    value &= SP;

    A = value;
    X = value;
    SP = value;

    Z = (value == 0);
    N = (value & 0x80) != 0;
}

void CPU65::anc(uint8_t value)
{
    A &= value;

    Z = (A == 0);
    N = (A & 0x80) != 0;

    C = N;
}

uint32_t CPU65::execute() {
    uint32_t cycles = 0;
    if (halted)
        return cycles;
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
            uint16_t addr = absIndexed(cycles, X, false);
            A = this->readMemory(cycles, addr);
            ldaSetFlags();
            break;
        }
        case INS_LDA_ABSY: // LDA Absolute,Y
        {
            uint16_t addr = absIndexed(cycles, Y, false);
            A = this->readMemory(cycles, addr);
            ldaSetFlags();
            break;
        }
        case INS_LDA_INDX: // LDA (Indirect,X)
        {
            uint16_t zpAddr = fetch(cycles);
            this->zeroPageAddX(cycles, zpAddr);
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            A = this->readMemory(cycles, addr);
            ldaSetFlags();
            break;
        }
        case INS_LDA_INDY: // LDA (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;
            
            A = this->readMemory(cycles, addr);
            ldaSetFlags();
            break;
        }
        case INS_ADC_IMM: // ADC Immediate
        {
            uint8_t value = fetch(cycles);
            adc(value);
            break;
        }
        case INS_ADC_ZP: // ADC Zero Page
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
            break;
        }
        case INS_ADC_ZPX: // ADC Zero Page,X
        {
            uint16_t addr = fetch(cycles);
            this->zeroPageAddX(cycles, addr);
            addr &= 0xFF; // Wrap around for zero page
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
            break;
        }
        case INS_ADC_ABS: // ADC Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
            break;
        }
        case INS_ADC_ABSX: // ADC Absolute,X
        {
            uint16_t addr = fetch16(cycles);
            this->zeroPageAddX(cycles, addr);
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
            break;
        }
        case INS_ADC_ABSY: // ADC Absolute,Y
        {
            uint16_t addr = fetch16(cycles);
            this->zeroPageAddY(cycles, addr);
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
            break;
        }
        case INS_ADC_INDX: // ADC (Indirect,X)
        {
            uint16_t zpAddr = fetch(cycles);
            this->zeroPageAddX(cycles, zpAddr);
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
            break;
        }
        case INS_ADC_INDY: // ADC (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;
            
            uint8_t value = this->readMemory(cycles, addr);
            adc(value);
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
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint8_t value = this->readMemory(cycles, addr);
            A &= value;
            andSetFlags();
            break;
        }
        case INS_AND_INDY: // AND (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;
            
            uint8_t value = this->readMemory(cycles, addr);
            A &= value;
            andSetFlags();
            break;
        }
        case INS_ASL_ACC: // ASL Accumulator
        {
            A = asl(A);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_ASL_ZP: // ASL Zero Page
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, asl(value)); // Write back to memory
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
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_BCS: // BCS (Branch if Carry Set)
        {
            int8_t offset = static_cast<int8_t>(fetch(cycles));
            if (C == 1) {
                PC += offset;
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_BEQ: // BEQ (Branch if Equal)
        {
            int8_t offset = static_cast<int8_t>(fetch(cycles));
            if (Z == 1) {
                PC += offset;
                cycles++; // Branch taken, increment cycles
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
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_BNE: // BNE (Branch if Not Equal)
        {
            int8_t offset = static_cast<int8_t>(fetch(cycles));
            if (Z == 0) {
                PC += offset;
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_BPL: // BPL (Branch if Positive)
        {
            int8_t offset = static_cast<int8_t>(fetch(cycles));
            if (N == 0) {
                PC += offset;
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_BRK: // BRK (Force Interrupt)
        {
            ++PC;

            push(cycles, static_cast<uint8_t>((PC >> 8) & 0xFF));
            push(cycles, static_cast<uint8_t>(PC & 0xFF));

            push(cycles, getStatus(true));


            I = 1;

            uint8_t low  = readMemory(cycles, 0xFFFE);
            uint8_t high = readMemory(cycles, 0xFFFF);

            PC = static_cast<uint16_t>(low)
            | (static_cast<uint16_t>(high) << 8);

            break;
        }
        case INS_BVC: // BVC (Branch if Overflow Clear)
        {
            int8_t offset = static_cast<int8_t>(fetch(cycles));
            if (V == 0) {
                PC += offset;
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_BVS: // BVS (Branch if Overflow Set)
        {
            int8_t offset = static_cast<int8_t>(fetch(cycles));
            if (V == 1) {
                PC += offset;
                cycles++; // Branch taken, increment cycles
            }
            break;
        }
        case INS_CLC: // CLC (Clear Carry Flag)
        {
            C = 0; // Clear Carry flag
            cycles++; // Increment cycles for the operation
            break;
        }
        case INS_CLD: // CLD (Clear Decimal Mode)
        {
            D = 0; // Clear Decimal Mode flag
            cycles++; // Increment cycles for the operation
            break;
        }
        case INS_CLI: // CLI (Clear Interrupt Disable)
        {
            I = 0; // Clear Interrupt Disable flag
            cycles++; // Increment cycles for the operation
            break;
        }
        case INS_CLV: // CLV (Clear Overflow Flag)
        {
            V = 0; // Clear Overflow flag
            cycles++; // Increment cycles for the operation
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
            uint16_t addr = absIndexed(cycles, X, false);
            uint8_t value = this->readMemory(cycles, addr);
            cmpSetFlags(value);
            break;
        }
        case INS_CMP_ABSY: // CMP Absolute,Y
        {
            uint16_t addr = absIndexed(cycles, Y, false);
            uint8_t value = this->readMemory(cycles, addr);
            cmpSetFlags(value);
            break;
        }
        case INS_CMP_INDX: // CMP (Indirect,X)
        {
            uint16_t zpAddr = fetch(cycles);
            this->zeroPageAddX(cycles, zpAddr);
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint8_t value = this->readMemory(cycles, addr);
            cmpSetFlags(value);
            break;
        }
        case INS_CMP_INDY: // CMP (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;
            
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
            cycles++; // Increment cycles for the operation
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
            cycles++; // Increment cycles for the operation
            decSetFlags(value);
            writeMemory(cycles, addr, value);
            break;
        }
        case INS_DEC_ABS: // DEC Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            value--;
            cycles++; // Increment cycles for the operation
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
            cycles++; // Increment cycles for the operation
            decSetFlags(value);
            writeMemory(cycles, addr, value);
            break;
        }
        case INS_DEX: // DEX (Decrement X Register)
        {
            X--;
            ++cycles; // Increment cycles for the operation
            dexSetFlags();
            break;
        }
        case INS_DEY: // DEY (Decrement Y Register)
        {
            Y--;
            ++cycles; // Increment cycles for the operation
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
            uint16_t addr = absIndexed(cycles, X, false);
            uint8_t value = this->readMemory(cycles, addr);
            A ^= value;
            eorSetFlags();
            break;
        }
        case INS_EOR_ABSY: // EOR Absolute,Y
        {
            uint16_t addr = absIndexed(cycles, Y, false);
            uint8_t value = this->readMemory(cycles, addr);
            A ^= value;
            eorSetFlags();
            break;
        }
        case INS_EOR_INDX: // EOR (Indirect,X)
        {
            uint16_t zpAddr = fetch(cycles);
            this->zeroPageAddX(cycles, zpAddr);
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint8_t value = this->readMemory(cycles, addr);
            A ^= value;
            eorSetFlags();
            break;
        }
        case INS_EOR_INDY: // EOR (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;

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
            ++cycles; // Increment cycles for the operation
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
            ++cycles; // Increment cycles for the operation
            incSetFlags(value);
            writeMemory(cycles, addr, value);
            break;
        }
        case INS_INC_ABS: // INC Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            value++;
            ++cycles; // Increment cycles for the operation
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
            ++cycles; // Increment cycles for the operation
            incSetFlags(value);
            writeMemory(cycles, addr, value);
            break;
        }
        case INS_INX: // INX (Increment X Register)
        {
            X++;
            ++cycles; // Increment cycles for the operation
            inxSetFlags();
            break;
        }
        case INS_INY: // INY (Increment Y Register)
        {
            Y++;
            ++cycles; // Increment cycles for the operation
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

            uint16_t highAddr =
                (addr & 0xFF00) | ((addr + 1) & 0x00FF);

            uint16_t targetAddr =
                readMemory(cycles, addr) |
                (static_cast<uint16_t>(readMemory(cycles, highAddr)) << 8);

            PC = targetAddr;

            break;
        }
        case INS_JSR: // JSR (Jump to Subroutine)
        {
            uint16_t addr = fetch16(cycles);
            uint16_t returnAddr = PC - 1; // Address of the next instruction after JSR
            ++cycles; // Increment cycles for the operation
            push(cycles, (returnAddr >> 8) & 0xFF); // Push high byte
            push(cycles, returnAddr & 0xFF);        // Push low byte
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
            uint16_t addr = absIndexed(cycles, Y, false);
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
            uint16_t addr = absIndexed(cycles, X, false);
            Y = this->readMemory(cycles, addr);
            ldySetFlags();
            break;
        }
        case INS_LSR_ACC: // LSR Accumulator
        {
            A = lsr(A);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_LSR_ZP: // LSR Zero Page
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = this->readMemory(cycles, addr);

            value = lsr(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_LSR_ZPX: // LSR Zero Page,X
        {
            uint16_t addr = fetch(cycles);
            this->zeroPageAddX(cycles, addr);
            addr &= 0xFF; // Wrap around for zero page

            uint8_t value = this->readMemory(cycles, addr);
            value = lsr(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_LSR_ABS: // LSR Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            value = lsr(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_LSR_ABSX: // LSR Absolute,X
        {
            uint16_t addr = fetch16(cycles);
            this->zeroPageAddX(cycles, addr);
            uint8_t value = this->readMemory(cycles, addr);
            value = lsr(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_NOP: // NOP (No Operation)
        {
            // Do nothing, just increment cycles
            ++cycles;
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
            uint16_t addr = absIndexed(cycles, X, false);
            uint8_t value = this->readMemory(cycles, addr);
            A |= value;
            oraSetFlags();
            break;
        }
        case INS_ORA_ABSY: // ORA Absolute,Y
        {
            uint16_t addr = absIndexed(cycles, Y, false);
            uint8_t value = this->readMemory(cycles, addr);
            A |= value;
            oraSetFlags();
            break;
        }
        case INS_ORA_INDX: // ORA (Indirect,X)
        {
            uint16_t zpAddr = fetch(cycles);
            this->zeroPageAddX(cycles, zpAddr);
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint8_t value = this->readMemory(cycles, addr);
            A |= value;
            oraSetFlags();
            break;
        }
        case INS_ORA_INDY: // ORA (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;
            
            uint8_t value = this->readMemory(cycles, addr);
            A |= value;
            oraSetFlags();
            break;
        }
        case INS_PHA: // PHA (Push Accumulator)
        {
            push(cycles, A);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_PHP: // PHP (Push Processor Status)
        {
            uint8_t status = (N << 7) | (V << 6) | (1 << 5) | (B << 4) | (D << 3) | (I << 2) | (Z << 1) | C;
            push(cycles, status);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_PLA: // PLA (Pull Accumulator)
        {
            A = pull(cycles);
            ++cycles; // Increment cycles for the operation
            ldaSetFlags();
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_PLP: // PLP (Pull Processor Status)
        {
            uint8_t status = pull(cycles);
            ++cycles; // Increment cycles for the operation
            plpSetFlags(status);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_ROL_ACC: // ROL Accumulator
        {
            A = rol(A);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_ROL_ZP: // ROL Zero Page
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, rol(value)); // Write back to memory
            break;
        }
        case INS_ROL_ZPX: // ROL Zero Page,X
        {
            uint16_t addr = fetch(cycles);
            this->zeroPageAddX(cycles, addr);
            addr &= 0xFF; // Wrap around for zero page
            uint8_t value = this->readMemory(cycles, addr);
            value = rol(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_ROL_ABS: // ROL Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            value = rol(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_ROL_ABSX: // ROL Absolute,X
        {
            uint16_t addr = fetch16(cycles);
            this->zeroPageAddX(cycles, addr);
            uint8_t value = this->readMemory(cycles, addr);
            value = rol(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_ROR_ACC: // ROR Accumulator
        {
            A = ror(A);
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_ROR_ZP: // ROR Zero Page   
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, ror(value)); // Write back to memory
            break;
        }
        case INS_ROR_ZPX: // ROR Zero Page,X
        {
            uint16_t addr = fetch(cycles);
            this->zeroPageAddX(cycles, addr);
            addr &= 0xFF; // Wrap around for zero page
            uint8_t value = this->readMemory(cycles, addr);
            value = ror(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_ROR_ABS: // ROR Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            value = ror(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_ROR_ABSX: // ROR Absolute,X
        {
            uint16_t addr = fetch16(cycles);
            this->zeroPageAddX(cycles, addr);
            uint8_t value = this->readMemory(cycles, addr);
            value = ror(value);
            ++cycles; // Increment cycles for the operation
            writeMemory(cycles, addr, value); // Write back to memory
            break;
        }
        case INS_RTI: // RTI (Return from Interrupt)
        {
            rtiSetFlags(cycles);
            uint8_t lowByte = pull(cycles);
            uint8_t highByte = pull(cycles);
            PC = (highByte << 8) | lowByte;
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_RTS: // RTS (Return from Subroutine)
        {
            uint8_t lowByte = pull(cycles);
            uint8_t highByte = pull(cycles);
            PC = ((highByte << 8) | lowByte) + 1; // Return to the instruction after JSR
            ++cycles; // Increment cycles for the operation
            ++cycles; // Increment cycles for the operation
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_SBC_IMM: // SBC Immediate
        {
            uint8_t value = fetch(cycles);
            sbc(value);
            break;
        }
        case INS_SBC_ZP: // SBC Zero Page
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SBC_ZPX: // SBC Zero Page,X
        {
            uint16_t addr = fetch(cycles);
            this->zeroPageAddX(cycles, addr);
            addr &= 0xFF; // Wrap around for zero page
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SBC_ABS: // SBC Absolute
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SBC_ABSX: // SBC Absolute,X
        {
            uint16_t addr = absIndexed(cycles, X, false);
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SBC_ABSY: // SBC Absolute,Y
        {
            uint16_t addr = absIndexed(cycles, Y, false);
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SBC_INDX: // SBC (Indirect,X)
        {
            uint16_t zpAddr = fetch(cycles);
            this->zeroPageAddX(cycles, zpAddr);
            zpAddr &= 0xFF; // Wrap around for zero page
            uint16_t addr = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SBC_INDY: // SBC (Indirect),Y
        {
            uint8_t zpAddr = fetch(cycles);
            uint16_t base = this->readMemory(cycles, zpAddr) | (this->readMemory(cycles, (zpAddr + 1) & 0xFF) << 8);
            
            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;
            
            uint8_t value = this->readMemory(cycles, addr);
            sbc(value);
            break;
        }
        case INS_SEC: // SEC (Set Carry Flag)
        {
            C = 1; // Set Carry flag
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_SED: // SED (Set Decimal Flag)
        {
            D = 1; // Set Decimal flag
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_SEI: // SEI (Set Interrupt Disable)
        {
            I = 1; // Set Interrupt Disable flag
            ++cycles; // Increment cycles for the operation
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
            zpAddr &= 0xFF; // Wrap around for zero page
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
            ++cycles; // Increment cycles for the operation
            taxSetFlags();
            break;
        }
        case INS_TAY: // TAY (Transfer Accumulator to Y)
        {
            Y = A;
            ++cycles; // Increment cycles for the operation
            taySetFlags();
            break;
        }
        case INS_TSX: // TSX (Transfer Stack Pointer to X)
        {
            X = SP;
            ++cycles; // Increment cycles for the operation
            tsxSetFlags();
            break;
        }
        case INS_TXA: // TXA (Transfer X to Accumulator)
        {
            A = X;
            ++cycles; // Increment cycles for the operation
            txaSetFlags();
            break;
        }
        case INS_TXS: // TXS (Transfer X to Stack Pointer)
        {
            SP = X;
            ++cycles; // Increment cycles for the operation
            break;
        }
        case INS_TYA: // TYA (Transfer Y to Accumulator)
        {
            A = Y;
            ++cycles; // Increment cycles for the operation
            tyaSetFlags();
            break;
        }
        case INS_LAX_ZP:
        {
            uint8_t addr = fetch(cycles);
            uint8_t value = readMemory(cycles, addr);

            A = value;
            X = value;

            Z = (value == 0);
            N = (value & 0x80) != 0;

            break;
        }
        case INS_LAX_ZPY:
        {
            uint8_t addr = fetch(cycles);
            addr += Y;

            uint8_t value = readMemory(cycles, addr);

            A = value;
            X = value;

            Z = (value == 0);
            N = (value & 0x80) != 0;

            break;
        }
        case INS_LAX_ABS:
        {
            uint16_t addr = fetch16(cycles);
            uint8_t value = readMemory(cycles, addr);

            A = value;
            X = value;

            Z = (value == 0);
            N = (value & 0x80) != 0;

            break;
        }
        case INS_LAX_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, false);
            uint8_t value = readMemory(cycles, addr);

            A = value;
            X = value;

            Z = (value == 0);
            N = (value & 0x80) != 0;

            break;
        }
        case INS_LAX_INDX:
        {
            uint8_t zpAddr = fetch(cycles);

            uint8_t ptr = static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(ptr + 1))
                ) << 8);

            uint8_t value = readMemory(cycles, addr);

            A = value;
            X = value;

            Z = (value == 0);
            N = (value & 0x80) != 0;

            break;
        }
        case INS_LAX_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zpAddr + 1))
                ) << 8);

            uint16_t addr = base + Y;

            if ((base & 0xFF00) != (addr & 0xFF00))
                ++cycles;

            uint8_t value = readMemory(cycles, addr);

            A = value;
            X = value;

            Z = (value == 0);
            N = (value & 0x80) != 0;

            break;
        }
        case INS_SAX_ZP:
        {
            uint8_t addr = fetch(cycles);
            writeMemory(cycles, addr, A & X);
            break;
        }
        case INS_SAX_ZPY:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + Y);
            ++cycles; // Increment cycles for the operation

            writeMemory(cycles, addr, A & X);
            break;
        }
        case INS_SAX_ABS:
        {
            uint16_t addr = fetch16(cycles);
            writeMemory(cycles, addr, A & X);
            break;
        }
        case INS_ISB_ZP:
        {
            uint8_t addr = fetch(cycles);
            isb(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_ISB_ZPX:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + X);
            cycles = 6;
            isb(addr, cycles);
            break;
        }
        case INS_ISB_ABS:
        {
            uint16_t addr = fetch16(cycles);
            isb(addr, cycles);
            cycles = 6;
            break;
        }
        case INS_ISB_ABSX:
        {   
            uint16_t addr = absIndexed(cycles, X, true);
            isb(addr, cycles);
            cycles = 7;
            break;
        }
        case INS_ISB_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, true);
            isb(addr, cycles);
            cycles = 7;
            break;
        }
        case INS_ISB_INDX:
        {
            uint8_t zpAddr = fetch(cycles);

            uint8_t ptr = static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(ptr + 1))
                ) << 8);

            isb(addr, cycles);
            cycles = 8;
            break;
        }
        case INS_ISB_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zpAddr + 1))
                ) << 8);

            uint16_t addr = base + Y;

            // Indexed indirect read-modify-write:
            // extra cycle regardless of page crossing.
            ++cycles;

            isb(addr, cycles);
            cycles = 8;
            break;
        }
        case INS_SLO_ZP:
        {
            uint8_t addr = fetch(cycles);
            slo(addr, cycles);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_SLO_ZPX:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + X);

            slo(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_SLO_ABS:
        {
            uint16_t addr = fetch16(cycles);
            slo(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_SLO_ABSX:
        {
            uint16_t addr = absIndexed(cycles, X, true);
            slo(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_SLO_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, true);
            slo(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_SLO_INDX:
        {
            uint8_t zpAddr = fetch(cycles);
            uint8_t ptr = static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(ptr + 1))
                ) << 8);

            slo(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_SLO_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zpAddr + 1))
                ) << 8);

            uint16_t addr = base + Y;

            ++cycles;

            slo(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_RLA_ZP:
        {
            uint8_t addr = fetch(cycles);
            rla(addr, cycles);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_RLA_ZPX:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + X);

            rla(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_RLA_ABS:
        {
            uint16_t addr = fetch16(cycles);
            rla(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_RLA_ABSX:
        {
            uint16_t addr = absIndexed(cycles, X, true);
            rla(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_RLA_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, true);
            rla(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_RLA_INDX:
        {
            uint8_t zpAddr = fetch(cycles);
            uint8_t ptr = static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(ptr + 1))
                ) << 8);

            rla(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_RLA_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zpAddr + 1))
                ) << 8);

            uint16_t addr = base + Y;

            ++cycles;

            rla(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_SRE_ZP:
        {
            uint8_t addr = fetch(cycles);
            sre(addr, cycles);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_SRE_ZPX:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + X);

            sre(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_SRE_ABS:
        {
            uint16_t addr = fetch16(cycles);
            sre(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_SRE_ABSX:
        {
            uint16_t addr = absIndexed(cycles, X, true);
            sre(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_SRE_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, true);
            sre(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_SRE_INDX:
        {
            uint8_t zpAddr = fetch(cycles);
            uint8_t ptr = static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(ptr + 1))
                ) << 8);

            sre(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_SRE_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zpAddr + 1))
                ) << 8);

            uint16_t addr = base + Y;

            // Read-modify-write: extra cycle
            ++cycles;

            sre(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_RRA_ZP:
        {
            uint8_t addr = fetch(cycles);
            rra(addr, cycles);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_RRA_ZPX:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + X);

            rra(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_RRA_ABS:
        {
            uint16_t addr = fetch16(cycles);
            rra(addr, cycles);
            cycles = 6; // Increment cycles for the operation
            break;
        }
        case INS_RRA_ABSX:
        {
            uint16_t addr = absIndexed(cycles, X, true);
            rra(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_RRA_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, true);
            rra(addr, cycles);
            cycles = 7; // Increment cycles for the operation
            break;
        }
        case INS_RRA_INDX:
        {
            uint8_t zpAddr = fetch(cycles);
            uint8_t ptr = static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(ptr + 1))
                ) << 8);

            rra(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_RRA_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zpAddr + 1))
                ) << 8);

            uint16_t addr = base + Y;

            // Read-modify-write: extra cycle
            ++cycles;

            rra(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_AHX_ABSY:
        {
            uint16_t base = fetch16(cycles);
            uint16_t addr = base + Y;

            uint8_t high = static_cast<uint8_t>(addr >> 8);

            ++cycles; // indexed store

            ahx(addr, high);
            break;
        }
        case INS_AHX_INDY:
        {
            uint8_t zp = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zp) |
                (static_cast<uint16_t>(
                    readMemory(cycles, static_cast<uint8_t>(zp + 1))
                ) << 8);

            uint16_t addr = base + Y;

            uint8_t high = static_cast<uint8_t>(addr >> 8);

            ++cycles; // (zp),Y store

            ahx(addr, high);
            break;
        }
        case INS_SHY_ABSX:
        {
            uint16_t base = fetch16(cycles);
            uint16_t addr = base + X;

            ++cycles; // indexed store

            shy(addr);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_SHX_ABSY:
        {
            uint16_t base = fetch16(cycles);
            uint16_t addr = base + Y;

            ++cycles; // indexed store

            shx(addr);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_TAS_ABSY:
        {
            uint16_t base = fetch16(cycles);
            uint16_t addr = base + Y;

            ++cycles; // indexed store

            tas(addr);
            cycles = 5; // Increment cycles for the operation
            break;
        }
        case INS_XAA_IMM:
        {
            uint8_t value = fetch(cycles);
            xaa(value);
            break;
        }
        case INS_DCP_ZP:
        {
            uint8_t addr = fetch(cycles);

            dcp(addr, cycles);

            break;
        }
        case INS_DCP_ZPX:
        {
            uint8_t addr = fetch(cycles);

            addr = static_cast<uint8_t>(addr + X);

            dcp(addr, cycles);

            break;
        }
        case INS_DCP_ABS:
        {
            uint16_t addr = fetch16(cycles);

            dcp(addr, cycles);

            break;
        }
        case INS_DCP_ABSX:
        {
            uint16_t addr = absIndexed(cycles, X, true);

            dcp(addr, cycles);

            break;
        }
        case INS_DCP_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, true);

            dcp(addr, cycles);

            break;
        }
        case INS_DCP_INDX:
        {
            uint8_t zpAddr = fetch(cycles);

            uint8_t ptr =
                static_cast<uint8_t>(zpAddr + X);

            uint16_t addr =
                readMemory(cycles, ptr) |
                (static_cast<uint16_t>(
                    readMemory(
                        cycles,
                        static_cast<uint8_t>(ptr + 1)
                    )
                ) << 8);

            dcp(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_DCP_INDY:
        {
            uint8_t zpAddr = fetch(cycles);

            uint16_t base =
                readMemory(cycles, zpAddr) |
                (static_cast<uint16_t>(
                    readMemory(
                        cycles,
                        static_cast<uint8_t>(zpAddr + 1)
                    )
                ) << 8);

            uint16_t addr = base + Y;

            ++cycles;

            dcp(addr, cycles);
            cycles = 8; // Increment cycles for the operation
            break;
        }
        case INS_ALR_IMM:
        {
            uint8_t value = fetch(cycles);
            alr(value);
            break;
        }
        case INS_ARR_IMM:
        {
            uint8_t value = fetch(cycles);
            arr(value);
            break;
        }
        case INS_AXS_IMM:
        {
            uint8_t value = fetch(cycles);
            axs(value);
            break;
        }
        case INS_LAS_ABSY:
        {
            uint16_t addr = absIndexed(cycles, Y, false);

            las(addr, cycles);

            break;
        }
        case INS_KIL_02:
        case INS_KIL_12:
        case INS_KIL_22:
        case INS_KIL_32:
        case INS_KIL_42:
        case INS_KIL_52:
        case INS_KIL_62:
        case INS_KIL_72:
        case INS_KIL_92:
        case INS_KIL_B2:
        case INS_KIL_D2:
        case INS_KIL_F2:
        {
            halted = true;
            break;
        }
        case INS_SBC_IMM_ILLEGAL:
        {
            uint8_t value = fetch(cycles);

            sbc(value);
            cycles = 2; // Increment cycles for the operation
            break;
        }
        case INS_ANC_IMM_0B:
        {
            uint8_t value = fetch(cycles);
            anc(value);
            break;
        }

        case INS_ANC_IMM_2B:
        {
            uint8_t value = fetch(cycles);
            anc(value);
            break;
        }
        case INS_NOP_1A:
        case INS_NOP_3A:
        case INS_NOP_5A:
        case INS_NOP_7A:
        case INS_NOP_DA:
        case INS_NOP_FA:
        {
            ++cycles;
            break;
        }
        case INS_NOP_IMM_80:
        case INS_NOP_IMM_82:
        case INS_NOP_IMM_89:
        case INS_NOP_IMM_C2:
        case INS_NOP_IMM_E2:
        {
            fetch(cycles);
            break;
        }
        case INS_NOP_ZP_04:
        case INS_NOP_ZP_44:
        case INS_NOP_ZP_64:
        {
            uint8_t addr = fetch(cycles);
            readMemory(cycles, addr);
            break;
        }
        case INS_NOP_ZPX_14:
        case INS_NOP_ZPX_34:
        case INS_NOP_ZPX_54:
        case INS_NOP_ZPX_74:
        case INS_NOP_ZPX_D4:
        case INS_NOP_ZPX_F4:
        {
            uint8_t addr = fetch(cycles);
            addr = static_cast<uint8_t>(addr + X);

            readMemory(cycles, addr);

            break;
        }
        case INS_NOP_ABS_0C:
        {
            uint16_t addr = fetch16(cycles);

            readMemory(cycles, addr);

            break;
        }
        case INS_NOP_ABSX_1C:
        case INS_NOP_ABSX_3C:
        case INS_NOP_ABSX_5C:
        case INS_NOP_ABSX_7C:
        case INS_NOP_ABSX_DC:
        case INS_NOP_ABSX_FC:
        {
            uint16_t addr = absIndexed(cycles, X, false);

            readMemory(cycles, addr);

            break;
        }
        
        default:
        {
            std::cerr << "Unknown instruction: " << std::hex << static_cast<int>(instruction) << std::endl;
            break;
        }
    }
    return cycles;
}