#include "CPU65.hh"

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

void CPU65::cpxSetFlags(uint8_t value)
{
    uint16_t result = static_cast<uint16_t>(X) - static_cast<uint16_t>(value);
    C = (X >= value) ? 1 : 0; // Set Carry flag
    Z = (result == 0); // Set Zero flag
    N = (result & 0x80) != 0; // Set Negative flag
}

void CPU65::cpySetFlags(uint8_t value)
{
    uint16_t result = static_cast<uint16_t>(Y) - static_cast<uint16_t>(value);
    C = (Y >= value) ? 1 : 0; // Set Carry flag
    Z = (result == 0); // Set Zero flag
    N = (result & 0x80) != 0; // Set Negative flag
}

void CPU65::decSetFlags(uint8_t value)
{
    Z = (value == 0); // Set Zero flag
    N = (value & 0x80) != 0; // Set Negative flag
}

void CPU65::dexSetFlags()
{
    Z = (X == 0); // Set Zero flag
    N = (X & 0x80) != 0; // Set Negative flag
}

void CPU65::deySetFlags()
{
    Z = (Y == 0); // Set Zero flag
    N = (Y & 0x80) != 0; // Set Negative flag
}

void CPU65::eorSetFlags()
{
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}

void CPU65::incSetFlags(uint8_t value)
{
    Z = (value == 0); // Set Zero flag
    N = (value & 0x80) != 0; // Set Negative flag
}

void CPU65::inxSetFlags()
{
    Z = (X == 0); // Set Zero flag
    N = (X & 0x80) != 0; // Set Negative flag
}

void CPU65::inySetFlags()
{
    Z = (Y == 0); // Set Zero flag
    N = (Y & 0x80) != 0; // Set Negative flag
}

void CPU65::ldxSetFlags()
{
    Z = (X == 0); // Set Zero flag
    N = (X & 0x80) != 0; // Set Negative flag
}

void CPU65::ldySetFlags()
{
    Z = (Y == 0); // Set Zero flag
    N = (Y & 0x80) != 0; // Set Negative flag
}

void CPU65::lsrSetFlags(uint32_t &cycles)
{
    C = (A & 0x01) ? 1 : 0; // Set Carry flag
    A >>= 1; // Shift right
    --cycles; // Decrement cycles for the operation
    Z = (A == 0); // Set Zero flag
    N = 0; // LSR always clears the Negative flag
}

void CPU65::oraSetFlags()
{
    Z = (A == 0); // Set Zero flag
    N = (A & 0x80) != 0; // Set Negative flag
}