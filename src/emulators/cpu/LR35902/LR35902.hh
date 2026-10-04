#pragma once

#include <cstdint>
#include <nlohmann/json.hpp>

#include "emulators/bus/Bus.hh"
#include "emulators/cpu/CPU.hh"

/**
 * @file LR35902.hh
 * @brief Sharp LR35902 (Game Boy) CPU emulation.
 */

class GameBoyBus;

struct LR35902State {
  uint8_t A, B, C, D, E, H, L;
  uint16_t PC, SP;
  uint8_t F;
  bool IME;
  uint8_t imeDelay;
  bool halted;
};

void to_json(nlohmann::json &j, const LR35902State &state);
void from_json(const nlohmann::json &j, LR35902State &state);

/**
 * @class LR35902
 * @brief Emulates the Sharp LR35902, the Game Boy's Z80-like 8-bit CPU.
 *
 * The CPU has eight 8-bit registers (A, F, B, C, D, E, H, L) that can be
 * paired as AF, BC, DE and HL, plus a 16-bit stack pointer (SP) and program
 * counter (PC). The F register holds the flags:
 *
 * | Bit | Flag | Meaning     |
 * |-----|------|-------------|
 * | 7   | Z    | Zero        |
 * | 6   | N    | Subtract    |
 * | 5   | H    | Half-carry  |
 * | 4   | C    | Carry       |
 *
 * Every instruction is implemented as a private method. Methods that access
 * memory take a `uint32_t &cycles` accumulator to which they add the elapsed
 * cycles; purely register-based instructions take no argument.
 *
 * @note The class inherits privately from CPU (`class LR35902 : CPU`). If it
 *       must be used polymorphically through a `CPU*`, use `public CPU`.
 */
class LR35902 : public CPU {
 public:
  /**
   * @brief Constructs the CPU.
   * @param bus Bus used for all memory accesses (not owned).
   */
  LR35902(GameBoyBus *bus);
  ~LR35902();

  /** @brief Resets registers, flags, IME and halt state to post-boot values. */
  void reset() override;

  /**
   * @brief Executes one instruction (or services one interrupt).
   * @return Number of cycles consumed.
   */
  uint32_t execute() override;

  /**
   * @brief Executes a CB-prefixed instruction.
   * @param opcode Second opcode byte (after 0xCB).
   * @param cycles Cycle accumulator, incremented by the instruction cost.
   */
  void executeCBInstruction(uint8_t opcode, uint32_t &cycles);

  /**
   * @brief Reads a byte from the bus.
   * @param cycles  Cycle accumulator, incremented for the access.
   * @param address 16-bit address.
   * @return Byte read.
   */
  uint8_t read(uint32_t &cycles, uint16_t address) override;

  /**
   * @brief Writes a byte to the bus.
   * @param cycles  Cycle accumulator, incremented for the access.
   * @param address 16-bit address.
   * @param value   Byte to write.
   */
  void write(uint32_t &cycles, uint16_t address, uint8_t value) override;

  /**
   * @brief Pops a 16-bit value from the stack (SP += 2).
   * @param cycles Cycle accumulator.
   * @return The popped value.
   */
  uint16_t popStack(uint32_t &cycles);

  /**
   * @brief Pushes a 16-bit value on the stack (SP -= 2).
   * @param cycles Cycle accumulator.
   * @param value  Value to push.
   * @return The pushed value.
   */
  uint16_t pushStack(uint32_t &cycles, uint16_t value);

  /** @brief Returns the current program counter (for debugging). */
  uint16_t getPC() { return PC; };

  void setState(const LR35902State &state);
  LR35902State getState() const;

 private:
  /// @name Registers
  /// @{
  uint8_t A, B, C, D, E, H, L;  ///< 8-bit general purpose registers.
  uint16_t PC, SP;              ///< Program counter and stack pointer.
  uint8_t F;                    ///< Flags register (Z N H C in bits 7-4).
  /// @}

  bool IME;              ///< Interrupt Master Enable flag.
  uint8_t imeDelay = 0;  ///< Delay for enabling IME after the EI instruction.
  bool halted;           ///< True while the CPU is in the HALT state.

  GameBoyBus *bus;  ///< System bus (not owned).

  /// @name Flag setters
  /// @{
  void setFlagZ(bool value);  ///< Sets/clears the Zero flag.
  void setFlagN(bool value);  ///< Sets/clears the Subtract flag.
  void setFlagH(bool value);  ///< Sets/clears the Half-carry flag.
  void setFlagC(bool value);  ///< Sets/clears the Carry flag.
  /// @}

  /// @name Flag getters
  /// @{
  uint8_t flagZ() const {
    return (F & 0x80) != 0;
  }  ///< @return 1 if Zero is set.
  uint8_t flagN() const {
    return (F & 0x40) != 0;
  }  ///< @return 1 if Subtract is set.
  uint8_t flagH() const {
    return (F & 0x20) != 0;
  }  ///< @return 1 if Half-carry is set.
  uint8_t flagC() const {
    return (F & 0x10) != 0;
  }  ///< @return 1 if Carry is set.
  /// @}

  // ---------------------------------------------------------------------
  // Instruction set methods
  // Naming: `_d8`/`_d16` immediate, `_a16` absolute address, `_r8` signed
  // relative offset, `_hl_ptr` = (HL), `hli`/`hld` = (HL+)/(HL-).
  // ---------------------------------------------------------------------

  /// @name Opcodes 0x00-0x3F: 16-bit loads, INC/DEC, rotates, JR, misc
  /// @{
  void ld_bc_d16(uint32_t &cycles);  ///< 0x01 LD BC,d16
  void ld_bc_a(uint32_t &cycles);    ///< 0x02 LD (BC),A
  void inc_bc(uint32_t &cycles);     ///< 0x03 INC BC
  void inc_b();                      ///< 0x04 INC B
  void dec_b();                      ///< 0x05 DEC B
  void ld_b_d8(uint32_t &cycles);    ///< 0x06 LD B,d8
  void rlca();  ///< 0x07 RLCA: rotate A left, bit 7 to carry
  void ld_a16_sp(uint32_t &cycles);  ///< 0x08 LD (a16),SP
  void add_hl_bc(uint32_t &cycles);  ///< 0x09 ADD HL,BC
  void ld_a_bc(uint32_t &cycles);    ///< 0x0A LD A,(BC)
  void dec_bc(uint32_t &cycles);     ///< 0x0B DEC BC
  void inc_c();                      ///< 0x0C INC C
  void dec_c();                      ///< 0x0D DEC C
  void ld_c_d8(uint32_t &cycles);    ///< 0x0E LD C,d8
  void rrca();  ///< 0x0F RRCA: rotate A right, bit 0 to carry
  void stop();  ///< 0x10 STOP
  void ld_de_d16(uint32_t &cycles);  ///< 0x11 LD DE,d16
  void ld_de_a(uint32_t &cycles);    ///< 0x12 LD (DE),A
  void inc_de(uint32_t &cycles);     ///< 0x13 INC DE
  void inc_d();                      ///< 0x14 INC D
  void dec_d();                      ///< 0x15 DEC D
  void ld_d_d8(uint32_t &cycles);    ///< 0x16 LD D,d8
  void rla();                        ///< 0x17 RLA: rotate A left through carry
  void jr_r8(uint32_t &cycles);      ///< 0x18 JR r8
  void add_hl_de(uint32_t &cycles);  ///< 0x19 ADD HL,DE
  void ld_a_de(uint32_t &cycles);    ///< 0x1A LD A,(DE)
  void dec_de(uint32_t &cycles);     ///< 0x1B DEC DE
  void inc_e();                      ///< 0x1C INC E
  void dec_e();                      ///< 0x1D DEC E
  void ld_e_d8(uint32_t &cycles);    ///< 0x1E LD E,d8
  void rra();                        ///< 0x1F RRA: rotate A right through carry
  void jr_nz_r8(uint32_t &cycles);   ///< 0x20 JR NZ,r8
  void ld_hl_d16(uint32_t &cycles);  ///< 0x21 LD HL,d16
  void ld_hli_a(uint32_t &cycles);   ///< 0x22 LD (HL+),A
  void inc_hl(uint32_t &cycles);     ///< 0x23 INC HL
  void inc_h();                      ///< 0x24 INC H
  void dec_h();                      ///< 0x25 DEC H
  void ld_h_d8(uint32_t &cycles);    ///< 0x26 LD H,d8
  void daa();                        ///< 0x27 DAA: decimal adjust A
  void jr_z_r8(uint32_t &cycles);    ///< 0x28 JR Z,r8
  void add_hl_hl(uint32_t &cycles);  ///< 0x29 ADD HL,HL
  void ld_a_hli(uint32_t &cycles);   ///< 0x2A LD A,(HL+)
  void dec_hl(uint32_t &cycles);     ///< 0x2B DEC HL
  void inc_l();                      ///< 0x2C INC L
  void dec_l();                      ///< 0x2D DEC L
  void ld_l_d8(uint32_t &cycles);    ///< 0x2E LD L,d8
  void cpl();                        ///< 0x2F CPL: complement A
  void jr_nc_r8(uint32_t &cycles);   ///< 0x30 JR NC,r8
  void ld_sp_d16(uint32_t &cycles);  ///< 0x31 LD SP,d16
  void ld_hl_ptr_minus_a(uint32_t &cycles);  ///< 0x32 LD (HL-),A
  void inc_sp(uint32_t &cycles);             ///< 0x33 INC SP
  void inc_hl_ptr(uint32_t &cycles);         ///< 0x34 INC (HL)
  void dec_hl_ptr(uint32_t &cycles);         ///< 0x35 DEC (HL)
  void ld_hl_d8(uint32_t &cycles);           ///< 0x36 LD (HL),d8
  void scf();                                ///< 0x37 SCF: set carry flag
  void jr_c_r8(uint32_t &cycles);            ///< 0x38 JR C,r8
  void add_hl_sp(uint32_t &cycles);          ///< 0x39 ADD HL,SP
  void ld_a_hld(uint32_t &cycles);           ///< 0x3A LD A,(HL-)
  void dec_sp(uint32_t &cycles);             ///< 0x3B DEC SP
  void inc_a();                              ///< 0x3C INC A
  void dec_a();                              ///< 0x3D DEC A
  void ld_a_d8(uint32_t &cycles);            ///< 0x3E LD A,d8
  void ccf();  ///< 0x3F CCF: complement carry flag
  /// @}

  /// @name Opcodes 0x40-0x7F: 8-bit register-to-register loads
  /// `ld_X_Y` copies Y into X. Variants with `_hl_ptr` read from (HL) and
  /// `ld_hl_Y` write Y to (HL); those take a cycle accumulator.
  /// @{
  void ld_b_b();
  void ld_b_c();
  void ld_b_d();
  void ld_b_e();
  void ld_b_h();
  void ld_b_l();
  void ld_b_hl_ptr(uint32_t &cycles);
  void ld_b_a();
  void ld_c_b();
  void ld_c_c();
  void ld_c_d();
  void ld_c_e();
  void ld_c_h();
  void ld_c_l();
  void ld_c_hl_ptr(uint32_t &cycles);
  void ld_c_a();
  void ld_d_b();
  void ld_d_c();
  void ld_d_d();
  void ld_d_e();
  void ld_d_h();
  void ld_d_l();
  void ld_d_hl_ptr(uint32_t &cycles);
  void ld_d_a();
  void ld_e_b();
  void ld_e_c();
  void ld_e_d();
  void ld_e_e();
  void ld_e_h();
  void ld_e_l();
  void ld_e_hl_ptr(uint32_t &cycles);
  void ld_e_a();
  void ld_h_b();
  void ld_h_c();
  void ld_h_d();
  void ld_h_e();
  void ld_h_h();
  void ld_h_l();
  void ld_h_hl_ptr(uint32_t &cycles);
  void ld_h_a();
  void ld_l_b();
  void ld_l_c();
  void ld_l_d();
  void ld_l_e();
  void ld_l_h();
  void ld_l_l();
  void ld_l_hl_ptr(uint32_t &cycles);
  void ld_l_a();
  void ld_hl_b(uint32_t &cycles);
  void ld_hl_c(uint32_t &cycles);
  void ld_hl_d(uint32_t &cycles);
  void ld_hl_e(uint32_t &cycles);
  void ld_hl_h(uint32_t &cycles);
  void ld_hl_l(uint32_t &cycles);
  void halt();  ///< 0x76 HALT: suspend the CPU until an interrupt is pending
  void ld_hl_a(uint32_t &cycles);
  void ld_a_b();
  void ld_a_c();
  void ld_a_d();
  void ld_a_e();
  void ld_a_h();
  void ld_a_l();
  void ld_a_hl_ptr(uint32_t &cycles);
  void ld_a_a();
  /// @}

  /// @name Opcodes 0x80-0xBF: 8-bit ALU operations on A
  /// Each family operates on A with operand B, C, D, E, H, L, (HL) or A.
  /// Flags are updated as per the LR35902 specification.
  /// @{
  // ADD A,r
  void add_a_b();
  void add_a_c();
  void add_a_d();
  void add_a_e();
  void add_a_h();
  void add_a_l();
  void add_a_hl_ptr(uint32_t &cycles);
  void add_a_a();
  // ADC A,r (add with carry)
  void adc_a_b();
  void adc_a_c();
  void adc_a_d();
  void adc_a_e();
  void adc_a_h();
  void adc_a_l();
  void adc_a_hl_ptr(uint32_t &cycles);
  void adc_a_a();
  // SUB A,r
  void sub_a_b();
  void sub_a_c();
  void sub_a_d();
  void sub_a_e();
  void sub_a_h();
  void sub_a_l();
  void sub_a_hl_ptr(uint32_t &cycles);
  void sub_a_a();
  // SBC A,r (subtract with carry)
  void sbc_a_b();
  void sbc_a_c();
  void sbc_a_d();
  void sbc_a_e();
  void sbc_a_h();
  void sbc_a_l();
  void sbc_a_hl_ptr(uint32_t &cycles);
  void sbc_a_a();
  // AND A,r
  void and_a_b();
  void and_a_c();
  void and_a_d();
  void and_a_e();
  void and_a_h();
  void and_a_l();
  void and_a_hl_ptr(uint32_t &cycles);
  void and_a_a();
  // XOR A,r
  void xor_a_b();
  void xor_a_c();
  void xor_a_d();
  void xor_a_e();
  void xor_a_h();
  void xor_a_l();
  void xor_a_hl_ptr(uint32_t &cycles);
  void xor_a_a();
  // OR A,r
  void or_a_b();
  void or_a_c();
  void or_a_d();
  void or_a_e();
  void or_a_h();
  void or_a_l();
  void or_a_hl_ptr(uint32_t &cycles);
  void or_a_a();
  // CP A,r (compare: subtract without storing the result)
  void cp_a_b();
  void cp_a_c();
  void cp_a_d();
  void cp_a_e();
  void cp_a_h();
  void cp_a_l();
  void cp_a_hl_ptr(uint32_t &cycles);
  void cp_a_a();
  /// @}

  /// @name Opcodes 0xC0-0xFF: control flow, stack, immediates, I/O
  /// @{
  void ret_nz(uint32_t &cycles);       ///< 0xC0 RET NZ
  void pop_bc(uint32_t &cycles);       ///< 0xC1 POP BC
  void jp_nz_a16(uint32_t &cycles);    ///< 0xC2 JP NZ,a16
  void jp_a16(uint32_t &cycles);       ///< 0xC3 JP a16
  void call_nz_a16(uint32_t &cycles);  ///< 0xC4 CALL NZ,a16
  void push_bc(uint32_t &cycles);      ///< 0xC5 PUSH BC
  void add_a_d8(uint32_t &cycles);     ///< 0xC6 ADD A,d8
  void rst_00(uint32_t &cycles);       ///< 0xC7 RST 00h
  void ret_z(uint32_t &cycles);        ///< 0xC8 RET Z
  void ret(uint32_t &cycles);          ///< 0xC9 RET
  void jp_z_a16(uint32_t &cycles);     ///< 0xCA JP Z,a16
  void prefix_bc(
      uint32_t &cycles);  ///< 0xCB PREFIX CB: fetch and run a CB instruction
  void call_z_a16(uint32_t &cycles);   ///< 0xCC CALL Z,a16
  void call_a16(uint32_t &cycles);     ///< 0xCD CALL a16
  void adc_a_d8(uint32_t &cycles);     ///< 0xCE ADC A,d8
  void rst_08(uint32_t &cycles);       ///< 0xCF RST 08h
  void ret_nc(uint32_t &cycles);       ///< 0xD0 RET NC
  void pop_de(uint32_t &cycles);       ///< 0xD1 POP DE
  void jp_nc_a16(uint32_t &cycles);    ///< 0xD2 JP NC,a16
  void call_nc_a16(uint32_t &cycles);  ///< 0xD4 CALL NC,a16
  void push_de(uint32_t &cycles);      ///< 0xD5 PUSH DE
  void sub_a_d8(uint32_t &cycles);     ///< 0xD6 SUB d8
  void rst_10(uint32_t &cycles);       ///< 0xD7 RST 10h
  void ret_c(uint32_t &cycles);        ///< 0xD8 RET C
  void reti(uint32_t &cycles);      ///< 0xD9 RETI: return and enable interrupts
  void jp_c_a16(uint32_t &cycles);  ///< 0xDA JP C,a16
  void call_c_a16(uint32_t &cycles);  ///< 0xDC CALL C,a16
  void sbc_a_d8(uint32_t &cycles);    ///< 0xDE SBC A,d8
  void rst_18(uint32_t &cycles);      ///< 0xDF RST 18h
  void ldh_a8_a(uint32_t &cycles);    ///< 0xE0 LDH (a8),A: write A to 0xFF00+a8
  void pop_hl(uint32_t &cycles);      ///< 0xE1 POP HL
  void ldh_c_a(uint32_t &cycles);     ///< 0xE2 LD (C),A: write A to 0xFF00+C
  void push_hl(uint32_t &cycles);     ///< 0xE5 PUSH HL
  void and_a_d8(uint32_t &cycles);    ///< 0xE6 AND d8
  void rst_20(uint32_t &cycles);      ///< 0xE7 RST 20h
  void add_sp_r8(uint32_t &cycles);   ///< 0xE8 ADD SP,r8
  void jp_hl(uint32_t &cycles);       ///< 0xE9 JP HL
  void ld_a16_a(uint32_t &cycles);    ///< 0xEA LD (a16),A
  void xor_a_d8(uint32_t &cycles);    ///< 0xEE XOR d8
  void rst_28(uint32_t &cycles);      ///< 0xEF RST 28h
  void ldh_a_a8(uint32_t &cycles);  ///< 0xF0 LDH A,(a8): read 0xFF00+a8 into A
  void pop_af(uint32_t &cycles);    ///< 0xF1 POP AF
  void ld_a_c(uint32_t &cycles);    ///< 0xF2 LD A,(C): read 0xFF00+C into A
                                    ///< (overload of ld_a_c())
  void di();                        ///< 0xF3 DI: disable interrupts
  void push_af(uint32_t &cycles);   ///< 0xF5 PUSH AF
  void or_a_d8(uint32_t &cycles);   ///< 0xF6 OR d8
  void rst_30(uint32_t &cycles);    ///< 0xF7 RST 30h
  void ld_hl_sp_plus_r8(uint32_t &cycles);  ///< 0xF8 LD HL,SP+r8
  void ld_sp_hl(uint32_t &cycles);          ///< 0xF9 LD SP,HL
  void ld_a_a16_ptr(uint32_t &cycles);      ///< 0xFA LD A,(a16)
  void ei();  ///< 0xFB EI: enable interrupts (delayed by one instruction)
  void cp_a_d8(uint32_t &cycles);  ///< 0xFE CP d8
  void rst_38(uint32_t &cycles);   ///< 0xFF RST 38h
  /// @}

  // ---------------------------------------------------------------------
  // CB-prefixed instruction set
  // Register operand order in the encoding: B, C, D, E, H, L, (HL), A.
  // ---------------------------------------------------------------------

  /// @name CB rotates and shifts (0xCB 0x00-0x3F)
  /// Variants ending in `_hl_ptr` operate on (HL) and take a cycle accumulator.
  /// @{
  // RLC r: rotate left, bit 7 to carry and bit 0
  void rlc_b();
  void rlc_c();
  void rlc_d();
  void rlc_e();
  void rlc_h();
  void rlc_l();
  void rlc_hl_ptr(uint32_t &cycles);
  void rlc_a();
  // RRC r: rotate right, bit 0 to carry and bit 7
  void rrc_b();
  void rrc_c();
  void rrc_d();
  void rrc_e();
  void rrc_h();
  void rrc_l();
  void rrc_hl_ptr(uint32_t &cycles);
  void rrc_a();
  // RL r: rotate left through carry
  void rl_b();
  void rl_c();
  void rl_d();
  void rl_e();
  void rl_h();
  void rl_l();
  void rl_hl_ptr(uint32_t &cycles);
  void rl_a();
  // RR r: rotate right through carry
  void rr_b();
  void rr_c();
  void rr_d();
  void rr_e();
  void rr_h();
  void rr_l();
  void rr_hl_ptr(uint32_t &cycles);
  void rr_a();
  // SLA r: arithmetic shift left, bit 0 = 0
  void sla_b();
  void sla_c();
  void sla_d();
  void sla_e();
  void sla_h();
  void sla_l();
  void sla_hl_ptr(uint32_t &cycles);
  void sla_a();
  // SRA r: arithmetic shift right, bit 7 preserved
  void sra_b();
  void sra_c();
  void sra_d();
  void sra_e();
  void sra_h();
  void sra_l();
  void sra_hl_ptr(uint32_t &cycles);
  void sra_a();
  // SWAP r: swap upper and lower nibbles
  void swap_b();
  void swap_c();
  void swap_d();
  void swap_e();
  void swap_h();
  void swap_l();
  void swap_hl_ptr(uint32_t &cycles);
  void swap_a();
  // SRL r: logical shift right, bit 7 = 0
  void srl_b();
  void srl_c();
  void srl_d();
  void srl_e();
  void srl_h();
  void srl_l();
  void srl_hl_ptr(uint32_t &cycles);
  void srl_a();
  /// @}

  /// @name CB bit operations (0xCB 0x40-0xFF)
  /// `bit` is the bit index (0-7) decoded from the opcode.
  /// - `bit_*`: BIT n,r  — set Z if bit n of the operand is 0 (0x40-0x7F)
  /// - `res_*`: RES n,r  — clear bit n of the operand (0x80-0xBF)
  /// - `set_*`: SET n,r  — set bit n of the operand (0xC0-0xFF)
  /// @{
  void bit_b_r(uint8_t bit);
  void bit_c_r(uint8_t bit);
  void bit_d_r(uint8_t bit);
  void bit_e_r(uint8_t bit);
  void bit_h_r(uint8_t bit);
  void bit_l_r(uint8_t bit);
  void bit_hl_ptr(uint8_t bit, uint32_t &cycles);
  void bit_a_r(uint8_t bit);
  void res_b_r(uint8_t bit);
  void res_c_r(uint8_t bit);
  void res_d_r(uint8_t bit);
  void res_e_r(uint8_t bit);
  void res_h_r(uint8_t bit);
  void res_l_r(uint8_t bit);
  void res_hl_ptr(uint8_t bit, uint32_t &cycles);
  void res_a_r(uint8_t bit);
  void set_b_r(uint8_t bit);
  void set_c_r(uint8_t bit);
  void set_d_r(uint8_t bit);
  void set_e_r(uint8_t bit);
  void set_h_r(uint8_t bit);
  void set_l_r(uint8_t bit);
  void set_hl_ptr(uint8_t bit, uint32_t &cycles);
  void set_a_r(uint8_t bit);
  /// @}
};
