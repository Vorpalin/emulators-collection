#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Bus.hh"
#include "CPU.hh"

/**
 * @file CPU65.hh
 * @brief MOS 6502-family CPU core (as used by the Atari 2600), including
 *        support for the common undocumented/"illegal" opcodes.
 */

/**
 * @class CPU65
 * @brief Emulates a MOS 6502-compatible CPU: registers, status flags,
 *        addressing modes, and the full documented + illegal instruction
 *        set, driven via a Bus for all memory access.
 */
class CPU65 : public CPU {
 public:
  /**
   * @brief Construct the CPU bound to a system bus.
   * @param bus Bus used for all memory reads/writes. Not owned.
   */
  CPU65(Bus *bus);

  /**
   * @brief Reset the CPU per the CPU interface (delegates to resetCPU()).
   */
  void reset() override;

  /**
   * @brief Reset registers, flags and the program counter (read from the
   *        reset vector) to their power-up state.
   */
  void resetCPU();

  /**
   * @brief Fetch, decode and execute the next instruction.
   * @return Number of CPU cycles consumed by the instruction.
   */
  uint32_t execute() override;

  /**
   * @brief Read a byte from memory via the bus, accounting for cycles.
   * @param cycles  Running cycle counter, incremented by this access.
   * @param address Address to read from.
   * @return The byte value at that address.
   */
  uint8_t readMemory(uint32_t &cycles, uint16_t address) override;

  /**
   * @brief Write a byte to memory via the bus, accounting for cycles.
   * @param cycles  Running cycle counter, incremented by this access.
   * @param address Address to write to.
   * @param value   Byte value to write.
   */
  void writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) override;

  /**
   * @brief Write a byte to memory via the bus without cycle accounting.
   * @param address Address to write to.
   * @param value   Byte value to write.
   */
  void writeMemory(uint16_t address, uint8_t value);

 private:
  Bus *bus;  ///< Pointer to the bus for memory access (not owned).

  uint8_t A;  ///< Accumulator.
  uint8_t X;  ///< Index Register X.
  uint8_t Y;  ///< Index Register Y.

  uint16_t SP;  ///< Stack Pointer.
  uint16_t PC;  ///< Program Counter.

  uint8_t C : 1;  ///< Carry Flag.
  uint8_t Z : 1;  ///< Zero Flag.
  uint8_t I : 1;  ///< Interrupt Disable Flag.
  uint8_t D : 1;  ///< Decimal Mode Flag.
  uint8_t B : 1;  ///< Break Command Flag.
  uint8_t V : 1;  ///< Overflow Flag.
  uint8_t N : 1;  ///< Negative Flag.
  bool halted;    ///< True if the CPU has hit a KIL/JAM instruction.

  /**
   * @brief Compute a zero-page,X address, wrapping within the zero page.
   * @param cycles  Running cycle counter, incremented by this addressing mode.
   * @param address In/out: base zero-page address, updated to the indexed
   * address.
   */
  void zeroPageAddX(uint32_t &cycles, uint16_t &address);

  /**
   * @brief Compute a zero-page,Y address, wrapping within the zero page.
   * @param cycles  Running cycle counter, incremented by this addressing mode.
   * @param address In/out: base zero-page address, updated to the indexed
   * address.
   */
  void zeroPageAddY(uint32_t &cycles, uint16_t &address);

  /**
   * @brief Fetch the byte at PC and advance PC by one.
   * @param cycles Running cycle counter, incremented by this fetch.
   * @return The fetched byte.
   */
  uint8_t fetch(uint32_t &cycles);

  /**
   * @brief Fetch a little-endian 16-bit word at PC and advance PC by two.
   * @param cycles Running cycle counter, incremented by this fetch.
   * @return The fetched 16-bit value.
   */
  uint16_t fetch16(uint32_t &cycles);

  /// @brief Update Z/N flags after an LDA.
  void ldaSetFlags();
  /**
   * @brief Update C/V/Z/N flags after an ADC.
   * @param value  Operand added to the accumulator.
   * @param result Full-width result of the addition, before truncation.
   */
  void adcSetFlags(uint8_t value, uint16_t result);
  /// @brief Update Z/N flags after an AND.
  void andSetFlags();
  /**
   * @brief Update C/Z/N flags after an ASL.
   * @param cycles Running cycle counter (unused for flag computation, kept for
   * symmetry).
   */
  void aslSetFlags(uint32_t &cycles);
  /**
   * @brief Update Z/V/N flags after a BIT test.
   * @param value Memory operand tested against the accumulator.
   */
  void bitSetFlags(uint8_t value);
  /**
   * @brief Update C/Z/N flags after a CMP.
   * @param value Memory operand compared against the accumulator.
   */
  void cmpSetFlags(uint8_t value);
  /**
   * @brief Update C/Z/N flags after a CPX.
   * @param value Memory operand compared against X.
   */
  void cpxSetFlags(uint8_t value);
  /**
   * @brief Update C/Z/N flags after a CPY.
   * @param value Memory operand compared against Y.
   */
  void cpySetFlags(uint8_t value);
  /**
   * @brief Update Z/N flags after a DEC.
   * @param value Decremented memory value.
   */
  void decSetFlags(uint8_t value);
  /// @brief Update Z/N flags after a DEX.
  void dexSetFlags();
  /// @brief Update Z/N flags after a DEY.
  void deySetFlags();
  /// @brief Update Z/N flags after an EOR.
  void eorSetFlags();
  /**
   * @brief Update Z/N flags after an INC.
   * @param value Incremented memory value.
   */
  void incSetFlags(uint8_t value);
  /// @brief Update Z/N flags after an INX.
  void inxSetFlags();
  /// @brief Update Z/N flags after an INY.
  void inySetFlags();
  /// @brief Update Z/N flags after an LDX.
  void ldxSetFlags();
  /// @brief Update Z/N flags after an LDY.
  void ldySetFlags();
  /**
   * @brief Update C/Z/N flags after an LSR.
   * @param cycles Running cycle counter (kept for symmetry with aslSetFlags).
   */
  void lsrSetFlags(uint32_t &cycles);
  /// @brief Update Z/N flags after an ORA.
  void oraSetFlags();
  /// @brief Update Z/N flags after a PLA.
  void plaSetFlags();
  /**
   * @brief Restore processor status flags after a PLP.
   * @param value Status byte pulled from the stack.
   */
  void plpSetFlags(uint8_t value);
  /**
   * @brief Update C/Z/N flags after a ROL.
   * @param cycles Running cycle counter (kept for symmetry).
   */
  void rolSetFlags(uint32_t &cycles);
  /**
   * @brief Update C/Z/N flags after a ROR.
   * @param cycles Running cycle counter (kept for symmetry).
   */
  void rorSetFlags(uint32_t &cycles);
  /**
   * @brief Restore flags and PC after an RTI.
   * @param cycles Running cycle counter, incremented by the stack pulls.
   */
  void rtiSetFlags(uint32_t &cycles);
  /**
   * @brief Update C/V/Z/N flags after an SBC.
   * @param oldA   Accumulator value before the subtraction.
   * @param value  Operand subtracted from the accumulator.
   * @param result Full-width result of the subtraction, before truncation.
   */
  void sbcSetFlags(uint8_t oldA, uint8_t value, uint16_t result);
  /// @brief Update Z/N flags after a TAX.
  void taxSetFlags();
  /// @brief Update Z/N flags after a TAY.
  void taySetFlags();
  /// @brief Update Z/N flags after a TSX.
  void tsxSetFlags();
  /// @brief Update Z/N flags after a TXA.
  void txaSetFlags();
  /// @brief Update Z/N flags after a TYA.
  void tyaSetFlags();

  /**
   * @brief Push a byte onto the stack and decrement SP.
   * @param cycles Running cycle counter, incremented by this access.
   * @param v      Byte value to push.
   */
  void push(uint32_t &cycles, uint8_t v);

  /**
   * @brief Increment SP and pull a byte from the stack.
   * @param cycles Running cycle counter, incremented by this access.
   * @return The pulled byte.
   */
  uint8_t pull(uint32_t &cycles);

  /**
   * @brief Arithmetic shift left, updating carry from the vacated bit.
   * @param v Value to shift.
   * @return Shifted value.
   */
  uint8_t asl(uint8_t v);

  /**
   * @brief Logical shift right, updating carry from the vacated bit.
   * @param v Value to shift.
   * @return Shifted value.
   */
  uint8_t lsr(uint8_t v);

  /**
   * @brief Rotate left through carry.
   * @param v Value to rotate.
   * @return Rotated value.
   */
  uint8_t rol(uint8_t v);

  /**
   * @brief Rotate right through carry.
   * @param v Value to rotate.
   * @return Rotated value.
   */
  uint8_t ror(uint8_t v);

  /**
   * @brief Add with carry into the accumulator, updating flags.
   * @param v Operand to add.
   */
  void adc(uint8_t v);

  /**
   * @brief Subtract with carry (borrow) from the accumulator, updating flags.
   * @param v Operand to subtract.
   */
  void sbc(uint8_t v);

  /**
   * @brief Resolve an absolute,X or absolute,Y indexed address, adding the
   *        extra page-crossing cycle when the access is a read.
   * @param cycles Running cycle counter, incremented as needed.
   * @param idx    Index register value (X or Y) to add to the base address.
   * @param write  True if this address is for a write (no penalty-cycle skip).
   * @return The resolved effective address.
   */
  uint16_t absIndexed(uint32_t &cycles, uint8_t idx, bool write);

  /**
   * @brief Perform a relative branch if @p cond is true, consuming extra
   *        cycles when the branch is taken and/or crosses a page.
   * @param cycles Running cycle counter, incremented as needed.
   * @param cond   Condition controlling whether the branch is taken.
   */
  void branch(uint32_t &cycles, bool cond);

  /**
   * @brief Build the processor status byte from the individual flags.
   * @param breakFlag Value to encode in the Break bit (differs for BRK/PHP vs
   * IRQ).
   * @return The packed status byte.
   */
  uint8_t getStatus(bool breakFlag) const;

  /// @name Illegal/undocumented opcode handlers
  /// @{
  /**
   * @brief ISC/ISB: increment memory then SBC.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void isb(uint16_t addr, uint32_t &cycles);
  /**
   * @brief SLO: shift memory left then OR into the accumulator.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void slo(uint16_t addr, uint32_t &cycles);
  /**
   * @brief RLA: rotate memory left then AND into the accumulator.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void rla(uint16_t addr, uint32_t &cycles);
  /**
   * @brief SRE: shift memory right then EOR into the accumulator.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void sre(uint16_t addr, uint32_t &cycles);
  /**
   * @brief RRA: rotate memory right then ADC into the accumulator.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void rra(uint16_t addr, uint32_t &cycles);
  /**
   * @brief AHX/SHA: store (A & X & high-byte-of-address+1) to memory.
   * @param addr High-order byte source for the unstable high-address AND.
   * @param high High byte of the effective address, used in the AND.
   */
  void ahx(uint16_t addr, uint8_t high);
  /**
   * @brief SHY: store (Y & high-byte-of-address+1) to memory.
   * @param addr Effective address to write to.
   */
  void shy(uint16_t addr);
  /**
   * @brief SHX: store (X & high-byte-of-address+1) to memory.
   * @param addr Effective address to write to.
   */
  void shx(uint16_t addr);
  /**
   * @brief TAS/SHS: transfer (A & X) to SP, then store (SP & high-byte+1).
   * @param addr Effective address to write to.
   */
  void tas(uint16_t addr);
  /**
   * @brief XAA/ANE: unstable AND of A, X and an immediate value.
   * @param value Immediate operand.
   */
  void xaa(uint8_t value);
  /**
   * @brief DCP: decrement memory then compare with the accumulator.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void dcp(uint16_t addr, uint32_t &cycles);
  /**
   * @brief ALR/ASR: AND accumulator with immediate, then LSR the result.
   * @param value Immediate operand.
   */
  void alr(uint8_t value);
  /**
   * @brief ARR: AND accumulator with immediate, then ROR with special flags.
   * @param value Immediate operand.
   */
  void arr(uint8_t value);
  /**
   * @brief AXS/SBX: AND A with X, subtract immediate, store in X.
   * @param value Immediate operand.
   */
  void axs(uint8_t value);
  /**
   * @brief LAS/LAR: AND memory with SP, load result into A, X and SP.
   * @param addr   Effective address of the memory operand.
   * @param cycles Running cycle counter, incremented by this access.
   */
  void las(uint16_t addr, uint32_t &cycles);
  /**
   * @brief ANC: AND accumulator with immediate, copy bit 7 into carry.
   * @param value Immediate operand.
   */
  void anc(uint8_t value);
  /// @}
};
