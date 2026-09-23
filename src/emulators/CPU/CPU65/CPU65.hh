#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CPU.hh"
#include "Bus.hh"

class CPU65 : public CPU {
public:
    CPU65(Bus* bus);

    void reset() override;

    uint32_t execute() override;

    uint8_t readMemory(uint32_t &cycles, uint16_t address) override;
    void writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) override;

    void writeMemory(uint16_t address, uint8_t value);
private:

    Bus* bus; // Pointer to the bus for memory access

    uint8_t A; // Accumulator
    uint8_t X; // Index Register X
    uint8_t Y; // Index Register Y
    
    uint16_t SP; // Stack Pointer
    uint16_t PC; // Program Counter
    
    uint8_t C : 1; // Carry Flag
    uint8_t Z : 1; // Zero Flag
    uint8_t I : 1; // Interrupt Disable Flag
    uint8_t D : 1; // Decimal Mode Flag
    uint8_t B : 1; // Break Command Flag
    uint8_t V : 1; // Overflow Flag
    uint8_t N : 1; // Negative Flag
    bool halted;

    void zeroPageAddX(uint32_t &cycles, uint16_t &address);
    void zeroPageAddY(uint32_t &cycles, uint16_t &address);
    uint8_t fetch(uint32_t &cycles);
    uint16_t fetch16(uint32_t &cycles);

    void ldaSetFlags();
    void adcSetFlags(uint8_t value,uint16_t result);
    void andSetFlags();
    void aslSetFlags(uint32_t &cycles);
    void bitSetFlags(uint8_t value);
    void cmpSetFlags(uint8_t value);
    void cpxSetFlags(uint8_t value);
    void cpySetFlags(uint8_t value);
    void decSetFlags(uint8_t value);
    void dexSetFlags();
    void deySetFlags();
    void eorSetFlags();
    void incSetFlags(uint8_t value);
    void inxSetFlags();
    void inySetFlags();
    void ldxSetFlags();
    void ldySetFlags();
    void lsrSetFlags(uint32_t &cycles);
    void oraSetFlags();
    void plaSetFlags();
    void plpSetFlags(uint8_t value);
    void rolSetFlags(uint32_t &cycles);
    void rorSetFlags(uint32_t &cycles);
    void rtiSetFlags(uint32_t &cycles);
    void sbcSetFlags(uint8_t oldA, uint8_t value, uint16_t result);
    void taxSetFlags();
    void taySetFlags();
    void tsxSetFlags();
    void txaSetFlags();
    void tyaSetFlags();

    void push(uint32_t &cycles, uint8_t v);
    uint8_t pull(uint32_t &cycles);

    uint8_t asl(uint8_t v);
    uint8_t lsr(uint8_t v);
    uint8_t rol(uint8_t v);
    uint8_t ror(uint8_t v);

    void adc(uint8_t v);
    void sbc(uint8_t v);

    uint16_t absIndexed(uint32_t &cycles, uint8_t idx, bool write);
    void branch(uint32_t &cycles, bool cond);
    uint8_t getStatus(bool breakFlag) const;

    void isb(uint16_t addr, uint32_t& cycles);
    void slo(uint16_t addr, uint32_t& cycles);
    void rla(uint16_t addr, uint32_t& cycles);
    void sre(uint16_t addr, uint32_t& cycles);
    void rra(uint16_t addr, uint32_t& cycles);
    void ahx(uint16_t addr, uint8_t high);
    void shy(uint16_t addr);
    void shx(uint16_t addr);
    void tas(uint16_t addr);
    void xaa(uint8_t value);
    void dcp(uint16_t addr, uint32_t& cycles);
    void alr(uint8_t value);
    void arr(uint8_t value);
    void axs(uint8_t value);
    void las(uint16_t addr, uint32_t& cycles);
    void anc(uint8_t value);
};