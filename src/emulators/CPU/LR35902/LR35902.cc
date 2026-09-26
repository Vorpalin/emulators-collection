#include "LR35902.hh"

#include "opcodes.hh"
#include "opcodesPrefix.hh"

LR35902::LR35902(Bus *bus) : CPU() {
  // Initialize registers and flags
  A = B = C = D = E = H = L = 0;
  F = 0;
  PC = 0;
  SP = 0xFFFE;  // Stack pointer initialized to top of stack
  IME = false;  // Interrupt Master Enable flag initialized to false
  this->bus = bus;
}

LR35902::~LR35902() {
  // Destructor logic if needed
}

void LR35902::reset() {
  // Reset registers and flags
  A = B = C = D = E = H = L = 0;
  F = 0;
  PC = 0;
  SP = 0xFFFE;   // Reset stack pointer
  IME = false;   // Reset interrupt master enable flag
  bus->reset();  // Reset the bus if needed
}

uint8_t LR35902::readMemory(uint32_t &cycles, uint16_t address) {
  // Read a byte from memory through the bus
  ++cycles;  // Increment cycles for the memory read operation
  return bus->readMemory(address);
}

void LR35902::writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) {
  // Write a byte to memory through the bus
  ++cycles;  // Increment cycles for the memory write operation
  bus->writeMemory(address, value);
}

uint16_t LR35902::popStack(uint32_t &cycles) {
  // Pop a 16-bit value from the stack
  uint8_t lowByte = readMemory(cycles, SP);   // Read low byte from stack
  SP++;                                       // Increment stack pointer
  uint8_t highByte = readMemory(cycles, SP);  // Read high byte from stack
  SP++;                                       // Increment stack pointer
  return (highByte << 8) | lowByte;  // Combine bytes into a 16-bit value
}

uint16_t LR35902::pushStack(uint32_t &cycles, uint16_t value) {
  // Push a 16-bit value onto the stack
  uint8_t highByte = (value >> 8) & 0xFF;  // Extract high byte
  uint8_t lowByte = value & 0xFF;          // Extract low byte
  SP--;                                    // Decrement stack pointer
  writeMemory(cycles, SP, highByte);       // Write high byte to stack
  SP--;                                    // Decrement stack pointer
  writeMemory(cycles, SP, lowByte);        // Write low byte to stack
  return value;                            // Return the pushed value
}

uint32_t LR35902::execute() {
  // Fetch the next instruction from memory
  uint32_t cycles = 0;
  uint8_t opcode = readMemory(cycles, PC);
  PC++;  // Increment program counter

  switch (opcode) {
    case NOP:  // NOP
      // No operation, just increment cycles
      break;

    case LD_BC_d16:  // LD BC, d16
      ld_bc_d16(cycles);
      break;
    case LD_BC_A:  // LD (BC), A
      ld_bc_a(cycles);
      break;
    case INC_BC:  // INC BC
      inc_bc(cycles);
      break;
    case INC_B:  // INC B
      inc_b();
      break;
    case DEC_B:  // DEC B
      dec_b();
      break;
    case LD_B_d8:  // LD B, d8
      ld_b_d8(cycles);
      break;
    case RLCA:  // RLCA
      rlca();
      break;
    case LD_a16_SP:  // LD (a16), SP
      ld_a16_sp(cycles);
      break;
    case ADD_HL_BC:  // ADD HL, BC
      add_hl_bc(cycles);
      break;
    case LD_A_BC:  // LD A, (BC)
      ld_a_bc(cycles);
      break;
    case DEC_BC:  // DEC BC
      dec_bc(cycles);
      break;
    case INC_C:  // INC C
      inc_c();
      break;
    case DEC_C:  // DEC C
      dec_c();
      break;
    case LD_C_d8:  // LD C, d8
      ld_c_d8(cycles);
      break;
    case RRCA:  // RRCA
      rrca();
      break;
    case STOP:  // STOP
      stop();
      break;
    case LD_DE_d16:  // LD DE, d16
      ld_de_d16(cycles);
      break;
    case LD_DE_A:  // LD (DE), A
      ld_de_a(cycles);
      break;
    case INC_DE:  // INC DE
      inc_de(cycles);
      break;
    case INC_D:  // INC D
      inc_d();
      break;
    case DEC_D:  // DEC D
      dec_d();
      break;
    case LD_D_d8:  // LD D, d8
      ld_d_d8(cycles);
      break;
    case RLA:  // RLA
      rla();
      break;
    case JR_r8:  // JR r8
      jr_r8(cycles);
      break;
    case ADD_HL_DE:  // ADD HL, DE
      add_hl_de(cycles);
      break;
    case LD_A_DE:  // LD A, (DE)
      ld_a_de(cycles);
      break;
    case DEC_DE:  // DEC DE
      dec_de(cycles);
      break;
    case INC_E:  // INC E
      inc_e();
      break;
    case DEC_E:  // DEC E
      dec_e();
      break;
    case LD_E_d8:  // LD E, d8
      ld_e_d8(cycles);
      break;
    case RRA:  // RRA
      rra();
      break;
    case JR_NZ_r8:  // JR NZ, r8
      jr_nz_r8(cycles);
      break;
    case LD_HL_d16:  // LD HL, d16
      ld_hl_d16(cycles);
      break;
    case LD_HLI_A:  // LD (HL+), A
      ld_hli_a(cycles);
      break;
    case INC_HL:  // INC HL
      inc_hl(cycles);
      break;
    case INC_H:  // INC H
      inc_h();
      break;
    case DEC_H:  // DEC H
      dec_h();
      break;
    case LD_H_d8:  // LD H, d8
      ld_h_d8(cycles);
      break;
    case DAA:  // DAA
      daa();
      break;
    case JR_Z_r8:  // JR Z, r8
      jr_z_r8(cycles);
      break;
    case ADD_HL_HL:  // ADD HL, HL
      add_hl_hl(cycles);
      break;
    case LD_A_HLI:  // LD A, (HL+)
      ld_a_hli(cycles);
      break;
    case DEC_HL:  // DEC HL
      dec_hl(cycles);
      break;
    case INC_L:  // INC L
      inc_l();
      break;
    case DEC_L:  // DEC L
      dec_l();
      break;
    case LD_L_d8:  // LD L, d8
      ld_l_d8(cycles);
      break;
    case CPL:  // CPL
      cpl();
      break;
    case JR_NC_r8:  // JR NC, r8
      jr_nc_r8(cycles);
      break;
    case LD_SP_d16:  // LD SP, d16
      ld_sp_d16(cycles);
      break;
    case LD_HLD_A:  // LD (HL-), A
      ld_hl_ptr_minus_a(cycles);
      break;
    case INC_SP:  // INC SP
      inc_sp(cycles);
      break;
    case INC_HL_PTR:  // INC (HL)
      inc_hl_ptr(cycles);
      break;
    case DEC_HL_PTR:  // DEC (HL)
      dec_hl_ptr(cycles);
      break;
    case LD_HL_d8:  // LD (HL), d8
      ld_hl_d8(cycles);
      break;
    case SCF:  // SCF
      scf();
      break;
    case JR_C_r8:  // JR C, r8
      jr_c_r8(cycles);
      break;
    case ADD_HL_SP:  // ADD HL, SP
      add_hl_sp(cycles);
      break;
    case LD_A_HLD:  // LD A, (HL-)
      ld_a_hld(cycles);
      break;
    case DEC_SP:  // DEC SP
      dec_sp(cycles);
      break;
    case INC_A:  // INC A
      inc_a();
      break;
    case DEC_A:  // DEC A
      dec_a();
      break;
    case LD_A_d8:  // LD A, d8
      ld_a_d8(cycles);
      break;
    case CCF:  // CCF
      ccf();
      break;
    case LD_B_B:  // LD B, B
      ld_b_b();
      break;
    case LD_B_C:  // LD B, C
      ld_b_c();
      break;
    case LD_B_D:  // LD B, D
      ld_b_d();
      break;
    case LD_B_E:  // LD B, E
      ld_b_e();
      break;
    case LD_B_H:  // LD B, H
      ld_b_h();
      break;
    case LD_B_L:  // LD B, L
      ld_b_l();
      break;
    case LD_B_HL:  // LD B, (HL)
      ld_b_hl_ptr(cycles);
      break;
    case LD_B_A:  // LD B, A
      ld_b_a();
      break;
    case LD_C_B:  // LD C, B
      ld_c_b();
      break;
    case LD_C_C:  // LD C, C
      ld_c_c();
      break;
    case LD_C_D:  // LD C, D
      ld_c_d();
      break;
    case LD_C_E:  // LD C, E
      ld_c_e();
      break;
    case LD_C_H:  // LD C, H
      ld_c_h();
      break;
    case LD_C_L:  // LD C, L
      ld_c_l();
      break;
    case LD_C_HL:  // LD C, (HL)
      ld_c_hl_ptr(cycles);
      break;
    case LD_C_A:  // LD C, A
      ld_c_a();
      break;
    case LD_D_B:  // LD D, B
      ld_d_b();
      break;
    case LD_D_C:  // LD D, C
      ld_d_c();
      break;
    case LD_D_D:  // LD D, D
      ld_d_d();
      break;
    case LD_D_E:  // LD D, E
      ld_d_e();
      break;
    case LD_D_H:  // LD D, H
      ld_d_h();
      break;
    case LD_D_L:  // LD D, L
      ld_d_l();
      break;
    case LD_D_HL:  // LD D, (HL)
      ld_d_hl_ptr(cycles);
      break;
    case LD_D_A:  // LD D, A
      ld_d_a();
      break;
    case LD_E_B:  // LD E, B
      ld_e_b();
      break;
    case LD_E_C:  // LD E, C
      ld_e_c();
      break;
    case LD_E_D:  // LD E, D
      ld_e_d();
      break;
    case LD_E_E:  // LD E, E
      ld_e_e();
      break;
    case LD_E_H:  // LD E, H
      ld_e_h();
      break;
    case LD_E_L:  // LD E, L
      ld_e_l();
      break;
    case LD_E_HL:  // LD E, (HL)
      ld_e_hl_ptr(cycles);
      break;
    case LD_E_A:  // LD E, A
      ld_e_a();
      break;
    case LD_H_B:  // LD H, B
      ld_h_b();
      break;
    case LD_H_C:  // LD H, C
      ld_h_c();
      break;
    case LD_H_D:  // LD H, D
      ld_h_d();
      break;
    case LD_H_E:  // LD H, E
      ld_h_e();
      break;
    case LD_H_H:  // LD H, H
      ld_h_h();
      break;
    case LD_H_L:  // LD H, L
      ld_h_l();
      break;
    case LD_H_HL:  // LD H, (HL)
      ld_h_hl_ptr(cycles);
      break;
    case LD_H_A:  // LD H, A
      ld_h_a();
      break;
    case LD_L_B:  // LD L, B
      ld_l_b();
      break;
    case LD_L_C:  // LD L, C
      ld_l_c();
      break;
    case LD_L_D:  // LD L, D
      ld_l_d();
      break;
    case LD_L_E:  // LD L, E
      ld_l_e();
      break;
    case LD_L_H:  // LD L, H
      ld_l_h();
      break;
    case LD_L_L:  // LD L, L
      ld_l_l();
      break;
    case LD_L_HL:  // LD L, (HL)
      ld_l_hl_ptr(cycles);
      break;
    case LD_L_A:  // LD L, A
      ld_l_a();
      break;
    case LD_HL_B:  // LD (HL), B
      ld_hl_b(cycles);
      break;
    case LD_HL_C:  // LD (HL), C
      ld_hl_c(cycles);
      break;
    case LD_HL_D:  // LD (HL), D
      ld_hl_d(cycles);
      break;
    case LD_HL_E:  // LD (HL), E
      ld_hl_e(cycles);
      break;
    case LD_HL_H:  // LD (HL), H
      ld_hl_h(cycles);
      break;
    case LD_HL_L:  // LD (HL), L
      ld_hl_l(cycles);
      break;
    case HALT:  // HALT
      halt();
      break;
    case LD_HL_A:  // LD (HL), A
      ld_hl_a(cycles);
      break;
    case LD_A_B:  // LD A, B
      ld_a_b();
      break;
    case LD_A_C:  // LD A, C
      ld_a_c();
      break;
    case LD_A_D:  // LD A, D
      ld_a_d();
      break;
    case LD_A_E:  // LD A, E
      ld_a_e();
      break;
    case LD_A_H:  // LD A, H
      ld_a_h();
      break;
    case LD_A_L:  // LD A, L
      ld_a_l();
      break;
    case LD_A_HL_PTR:  // LD A, (HL)
      ld_a_hl_ptr(cycles);
      break;
    case LD_A_A:  // LD A, A
      ld_a_a();
      break;
    case ADD_A_B:  // ADD A, B
      add_a_b();
      break;
    case ADD_A_C:  // ADD A, C
      add_a_c();
      break;
    case ADD_A_D:  // ADD A, D
      add_a_d();
      break;
    case ADD_A_E:  // ADD A, E
      add_a_e();
      break;
    case ADD_A_H:  // ADD A, H
      add_a_h();
      break;
    case ADD_A_L:  // ADD A, L
      add_a_l();
      break;
    case ADD_A_HL:  // ADD A, (HL)
      add_a_hl_ptr(cycles);
      break;
    case ADD_A_A:  // ADD A, A
      add_a_a();
      break;
    case ADC_A_B:  // ADC A, B
      adc_a_b();
      break;
    case ADC_A_C:  // ADC A, C
      adc_a_c();
      break;
    case ADC_A_D:  // ADC A, D
      adc_a_d();
      break;
    case ADC_A_E:  // ADC A, E
      adc_a_e();
      break;
    case ADC_A_H:  // ADC A, H
      adc_a_h();
      break;
    case ADC_A_L:  // ADC A, L
      adc_a_l();
      break;
    case ADC_A_HL:  // ADC A, (HL)
      adc_a_hl_ptr(cycles);
      break;
    case ADC_A_A:  // ADC A, A
      adc_a_a();
      break;
    case SUB_A_B:  // SUB A, B
      sub_a_b();
      break;
    case SUB_A_C:  // SUB A, C
      sub_a_c();
      break;
    case SUB_A_D:  // SUB A, D
      sub_a_d();
      break;
    case SUB_A_E:  // SUB A, E
      sub_a_e();
      break;
    case SUB_A_H:  // SUB A, H
      sub_a_h();
      break;
    case SUB_A_L:  // SUB A, L
      sub_a_l();
      break;
    case SUB_A_HL:  // SUB A, (HL)
      sub_a_hl_ptr(cycles);
      break;
    case SUB_A_A:  // SUB A, A
      sub_a_a();
      break;
    case SBC_A_B:  // SBC A, B
      sbc_a_b();
      break;
    case SBC_A_C:  // SBC A, C
      sbc_a_c();
      break;
    case SBC_A_D:  // SBC A, D
      sbc_a_d();
      break;
    case SBC_A_E:  // SBC A, E
      sbc_a_e();
      break;
    case SBC_A_H:  // SBC A, H
      sbc_a_h();
      break;
    case SBC_A_L:  // SBC A, L
      sbc_a_l();
      break;
    case SBC_A_HL:  // SBC A, (HL)
      sbc_a_hl_ptr(cycles);
      break;
    case SBC_A_A:  // SBC A, A
      sbc_a_a();
      break;
    case AND_A_B:  // AND A, B
      and_a_b();
      break;
    case AND_A_C:  // AND A, C
      and_a_c();
      break;
    case AND_A_D:  // AND A, D
      and_a_d();
      break;
    case AND_A_E:  // AND A, E
      and_a_e();
      break;
    case AND_A_H:  // AND A, H
      and_a_h();
      break;
    case AND_A_L:  // AND A, L
      and_a_l();
      break;
    case AND_A_HL:  // AND A, (HL)
      and_a_hl_ptr(cycles);
      break;
    case AND_A_A:  // AND A, A
      and_a_a();
      break;
    case XOR_A_B:  // XOR A, B
      xor_a_b();
      break;
    case XOR_A_C:  // XOR A, C
      xor_a_c();
      break;
    case XOR_A_D:  // XOR A, D
      xor_a_d();
      break;
    case XOR_A_E:  // XOR A, E
      xor_a_e();
      break;
    case XOR_A_H:  // XOR A, H
      xor_a_h();
      break;
    case XOR_A_L:  // XOR A, L
      xor_a_l();
      break;
    case XOR_A_HL:  // XOR A, (HL)
      xor_a_hl_ptr(cycles);
      break;
    case XOR_A_A:  // XOR A, A
      xor_a_a();
      break;
    case OR_A_B:  // OR A, B
      or_a_b();
      break;
    case OR_A_C:  // OR A, C
      or_a_c();
      break;
    case OR_A_D:  // OR A, D
      or_a_d();
      break;
    case OR_A_E:  // OR A, E
      or_a_e();
      break;
    case OR_A_H:  // OR A, H
      or_a_h();
      break;
    case OR_A_L:  // OR A, L
      or_a_l();
      break;
    case OR_A_HL:  // OR A, (HL)
      or_a_hl_ptr(cycles);
      break;
    case OR_A_A:  // OR A, A
      or_a_a();
      break;
    case CP_A_B:  // CP A, B
      cp_a_b();
      break;
    case CP_A_C:  // CP A, C
      cp_a_c();
      break;
    case CP_A_D:  // CP A, D
      cp_a_d();
      break;
    case CP_A_E:  // CP A, E
      cp_a_e();
      break;
    case CP_A_H:  // CP A, H
      cp_a_h();
      break;
    case CP_A_L:  // CP A, L
      cp_a_l();
      break;
    case CP_A_HL:  // CP A, (HL)
      cp_a_hl_ptr(cycles);
      break;
    case CP_A_A:  // CP A, A
      cp_a_a();
      break;
    case RET_Z:  // RET Z
      ret_z(cycles);
      break;
    case RET:  // RET
      ret(cycles);
      break;
    case JP_Z_a16:  // JP Z, a16
      jp_z_a16(cycles);
      break;
    case PREFIX_CB:  // PREFIX CB
      prefix_bc(cycles);
      break;
    case CALL_Z_a16:  // CALL Z, a16
      call_z_a16(cycles);
      break;
    case CALL_a16:  // CALL a16
      call_a16(cycles);
      break;
    case ADC_A_d8:  // ADC A, d8
      adc_a_d8(cycles);
      break;
    case RST_08H:  // RST 08H
      rst_08(cycles);
      break;
    case RET_NC:  // RST NC
      ret_nc(cycles);
      break;
    case POP_DE:  // POP DE
      pop_de(cycles);
      break;
    case JP_NC_a16:  // JP NC, a16
      jp_nc_a16(cycles);
      break;
    case CALL_NC_a16:  // CALL NC, a16
      call_nc_a16(cycles);
      break;
    case PUSH_DE:  // PUSH DE
      push_de(cycles);
      break;
    case SUB_A_d8:  // SUB A, d8
      sub_a_d8(cycles);
      break;
    case RST_10H:  // RST 10H
      rst_10(cycles);
      break;
    case RET_C:  // RET C
      ret_c(cycles);
      break;
    case RETI:  // RETI
      reti(cycles);
      break;
    case JP_C_a16:  // JP C, a16
      jp_c_a16(cycles);
      break;
    case CALL_C_a16:  // CALL C, a16
      call_c_a16(cycles);
      break;
    case SBC_A_d8:  // SBC A, d8
      sbc_a_d8(cycles);
      break;
    case RST_18H:  // RST 18H
      rst_18(cycles);
      break;
    case LDH_a8_A:  // LDH (a8), A
      ldh_a8_a(cycles);
      break;
    case POP_HL:  // POP HL
      pop_hl(cycles);
      break;
    case LDH_C_A:  // LDH (C), A
      ldh_c_a(cycles);
      break;
    case PUSH_HL:  // PUSH HL
      push_hl(cycles);
      break;
    case AND_A_d8:  // AND A, d8
      and_a_d8(cycles);
      break;
    case RST_20H:  // RST 20H
      rst_20(cycles);
      break;
    case ADD_SP_r8:  // ADD SP, r8
      add_sp_r8(cycles);
      break;
    case JP_HL:  // JP (HL)
      jp_hl(cycles);
      break;
    case LD_a16_A:  // LD (a16), A
      ld_a16_a(cycles);
      break;
    case XOR_A_d8:  // XOR A, d8
      xor_a_d8(cycles);
      break;
    case RST_28H:  // RST 28H
      rst_28(cycles);
      break;
    case LDH_A_a8:  // LDH A, (a8)
      ldh_a_a8(cycles);
      break;
    case POP_AF:  // POP AF
      pop_af(cycles);
      break;
    case LDH_A_C:  // LDH A, (C)
      ld_a_c(cycles);
      break;
    case PUSH_AF:  // PUSH AF
      push_af(cycles);
      break;
    case OR_A_d8:  // OR A, d8
      or_a_d8(cycles);
      break;
    case RST_30H:  // RST 30H
      rst_30(cycles);
      break;
    case LDHL_SP_r8:  // LD HL, SP+r8
      ld_hl_sp_plus_r8(cycles);
      break;
    case LD_SP_HL:  // LD SP, HL
      ld_sp_hl(cycles);
      break;
    case LD_A_a16:  // LD A, (a16)
      ld_a_a16_ptr(cycles);
      break;
    case EI:  // EI
      ei();
      break;
    case CP_A_d8:  // CP A, d8
      cp_a_d8(cycles);
      break;
    case RST_38H:  // RST 38H
      rst_38(cycles);
      break;
    default:
      break;
  }

  return cycles;  // Return the number of cycles taken for this instruction
}

void LR35902::executeCBInstruction(uint8_t opcode, uint32_t &cycles) {
  switch (opcode) {
    case RLC_B:  // RLC B
      rlc_b();
      break;
    case RLC_C:  // RLC C
      rlc_c();
      break;
    case RLC_D:  // RLC D
      rlc_d();
      break;
    case RLC_E:  // RLC E
      rlc_e();
      break;
    case RLC_H:  // RLC H
      rlc_h();
      break;
    case RLC_L:  // RLC L
      rlc_l();
      break;
    case RLC_HL:  // RLC (HL)
      rlc_hl_ptr(cycles);
      break;
    case RLC_A:  // RLC A
      rlc_a();
      break;
    case RRC_B:  // RRC B
      rrc_b();
      break;
    case RRC_C:  // RRC C
      rrc_c();
      break;
    case RRC_D:  // RRC D
      rrc_d();
      break;
    case RRC_E:  // RRC E
      rrc_e();
      break;
    case RRC_H:  // RRC H
      rrc_h();
      break;
    case RRC_L:  // RRC L
      rrc_l();
      break;
    case RRC_HL:  // RRC (HL)
      rrc_hl_ptr(cycles);
      break;
    case RRC_A:  // RRC A
      rrc_a();
      break;
    case RL_B:  // RL B
      rl_b();
      break;
    case RL_C:  // RL C
      rl_c();
      break;
    case RL_D:  // RL D
      rl_d();
      break;
    case RL_E:  // RL E
      rl_e();
      break;
    case RL_H:  // RL H
      rl_h();
      break;
    case RL_L:  // RL L
      rl_l();
      break;
    case RL_HL:  // RL (HL)
      rl_hl_ptr(cycles);
      break;
    case RL_A:  // RL A
      rl_a();
      break;
    case RR_B:  // RR B
      rr_b();
      break;
    case RR_C:  // RR C
      rr_c();
      break;
    case RR_D:  // RR D
      rr_d();
      break;
    case RR_E:  // RR E
      rr_e();
      break;
    case RR_H:  // RR H
      rr_h();
      break;
    case RR_L:  // RR L
      rr_l();
      break;
    case RR_HL:  // RR (HL)
      rr_hl_ptr(cycles);
      break;
    case RR_A:  // RR A
      rr_a();
      break;
    case SLA_B:  // SLA B
      sla_b();
      break;
    case SLA_C:  // SLA C
      sla_c();
      break;
    case SLA_D:  // SLA D
      sla_d();
      break;
    case SLA_E:  // SLA E
      sla_e();
      break;
    case SLA_H:  // SLA H
      sla_h();
      break;
    case SLA_L:  // SLA L
      sla_l();
      break;
    case SLA_HL:  // SLA (HL)
      sla_hl_ptr(cycles);
      break;
    case SLA_A:  // SLA A
      sla_a();
      break;
    case SRA_B:  // SRA B
      sra_b();
      break;
    case SRA_C:  // SRA C
      sra_c();
      break;
    case SRA_D:  // SRA D
      sra_d();
      break;
    case SRA_E:  // SRA E
      sra_e();
      break;
    case SRA_H:  // SRA H
      sra_h();
      break;
    case SRA_L:  // SRA L
      sra_l();
      break;
    case SRA_HL:  // SRA (HL)
      sra_hl_ptr(cycles);
      break;
    case SRA_A:  // SRA A
      sra_a();
      break;
    case SWAP_B:  // SWAP B
      swap_b();
      break;
    case SWAP_C:  // SWAP C
      swap_c();
      break;
    case SWAP_D:  // SWAP D
      swap_d();
      break;
    case SWAP_E:  // SWAP E
      swap_e();
      break;
    case SWAP_H:  // SWAP H
      swap_h();
      break;
    case SWAP_L:  // SWAP L
      swap_l();
      break;
    case SWAP_HL:  // SWAP (HL)
      swap_hl_ptr(cycles);
      break;
    case SWAP_A:  // SWAP A
      swap_a();
      break;
    case SRL_B:  // SRL B
      srl_b();
      break;
    case SRL_C:  // SRL C
      srl_c();
      break;
    case SRL_D:  // SRL D
      srl_d();
      break;
    case SRL_E:  // SRL E
      srl_e();
      break;
    case SRL_H:  // SRL H
      srl_h();
      break;
    case SRL_L:  // SRL L
      srl_l();
      break;
    case SRL_HL:  // SRL (HL)
      srl_hl_ptr(cycles);
      break;
    case SRL_A:  // SRL A
      srl_a();
      break;
    case BIT_0_B:  // BIT 0, B
      bit_b_r(0);
      break;
    case BIT_0_C:  // BIT 0, C
      bit_c_r(0);
      break;
    case BIT_0_D:  // BIT 0, D
      bit_d_r(0);
      break;
    case BIT_0_E:  // BIT 0, E
      bit_e_r(0);
      break;
    case BIT_0_H:  // BIT 0, H
      bit_h_r(0);
      break;
    case BIT_0_L:  // BIT 0, L
      bit_l_r(0);
      break;
    case BIT_0_HL:  // BIT 0, (HL)
      bit_hl_ptr(0, cycles);
      break;
    case BIT_0_A:  // BIT 0, A
      bit_a_r(0);
      break;
    case BIT_1_B:  // BIT 1, B
      bit_b_r(1);
      break;
    case BIT_1_C:  // BIT 1, C
      bit_c_r(1);
      break;
    case BIT_1_D:  // BIT 1, D
      bit_d_r(1);
      break;
    case BIT_1_E:  // BIT 1, E
      bit_e_r(1);
      break;
    case BIT_1_H:  // BIT 1, H
      bit_h_r(1);
      break;
    case BIT_1_L:  // BIT 1, L
      bit_l_r(1);
      break;
    case BIT_1_HL:  // BIT 1, (HL)
      bit_hl_ptr(1, cycles);
      break;
    case BIT_1_A:  // BIT 1, A
      bit_a_r(1);
      break;
    case BIT_2_B:  // BIT 2, B
      bit_b_r(2);
      break;
    case BIT_2_C:  // BIT 2, C
      bit_c_r(2);
      break;
    case BIT_2_D:  // BIT 2, D
      bit_d_r(2);
      break;
    case BIT_2_E:  // BIT 2, E
      bit_e_r(2);
      break;
    case BIT_2_H:  // BIT 2, H
      bit_h_r(2);
      break;
    case BIT_2_L:  // BIT 2, L
      bit_l_r(2);
      break;
    case BIT_2_HL:  // BIT 2, (HL)
      bit_hl_ptr(2, cycles);
      break;
    case BIT_2_A:  // BIT 2, A
      bit_a_r(2);
      break;
    case BIT_3_B:  // BIT 3, B
      bit_b_r(3);
      break;
    case BIT_3_C:  // BIT 3, C
      bit_c_r(3);
      break;
    case BIT_3_D:  // BIT 3, D
      bit_d_r(3);
      break;
    case BIT_3_E:  // BIT 3, E
      bit_e_r(3);
      break;
    case BIT_3_H:  // BIT 3, H
      bit_h_r(3);
      break;
    case BIT_3_L:  // BIT 3, L
      bit_l_r(3);
      break;
    case BIT_3_HL:  // BIT 3, (HL)
      bit_hl_ptr(3, cycles);
      break;
    case BIT_3_A:  // BIT 3, A
      bit_a_r(3);
      break;
    case BIT_4_B:  // BIT 4, B
      bit_b_r(4);
      break;
    case BIT_4_C:  // BIT 4, C
      bit_c_r(4);
      break;
    case BIT_4_D:  // BIT 4, D
      bit_d_r(4);
      break;
    case BIT_4_E:  // BIT 4, E
      bit_e_r(4);
      break;
    case BIT_4_H:  // BIT 4, H
      bit_h_r(4);
      break;
    case BIT_4_L:  // BIT 4, L
      bit_l_r(4);
      break;
    case BIT_4_HL:  // BIT 4, (HL)
      bit_hl_ptr(4, cycles);
      break;
    case BIT_4_A:  // BIT 4, A
      bit_a_r(4);
      break;
    case BIT_5_B:  // BIT 5, B
      bit_b_r(5);
      break;
    case BIT_5_C:  // BIT 5, C
      bit_c_r(5);
      break;
    case BIT_5_D:  // BIT 5, D
      bit_d_r(5);
      break;
    case BIT_5_E:  // BIT 5, E
      bit_e_r(5);
      break;
    case BIT_5_H:  // BIT 5, H
      bit_h_r(5);
      break;
    case BIT_5_L:  // BIT 5, L
      bit_l_r(5);
      break;
    case BIT_5_HL:  // BIT 5, (HL)
      bit_hl_ptr(5, cycles);
      break;
    case BIT_5_A:  // BIT 5, A
      bit_a_r(5);
      break;
    case BIT_6_B:  // BIT 6, B
      bit_b_r(6);
      break;
    case BIT_6_C:  // BIT 6, C
      bit_c_r(6);
      break;
    case BIT_6_D:  // BIT 6, D
      bit_d_r(6);
      break;
    case BIT_6_E:  // BIT 6, E
      bit_e_r(6);
      break;
    case BIT_6_H:  // BIT 6, H
      bit_h_r(6);
      break;
    case BIT_6_L:  // BIT 6, L
      bit_l_r(6);
      break;
    case BIT_6_HL:  // BIT 6, (HL)
      bit_hl_ptr(6, cycles);
      break;
    case BIT_6_A:  // BIT 6, A
      bit_a_r(6);
      break;
    case BIT_7_B:  // BIT 7, B
      bit_b_r(7);
      break;
    case BIT_7_C:  // BIT 7, C
      bit_c_r(7);
      break;
    case BIT_7_D:  // BIT 7, D
      bit_d_r(7);
      break;
    case BIT_7_E:  // BIT 7, E
      bit_e_r(7);
      break;
    case BIT_7_H:  // BIT 7, H
      bit_h_r(7);
      break;
    case BIT_7_L:  // BIT 7, L
      bit_l_r(7);
      break;
    case BIT_7_HL:  // BIT 7, (HL)
      bit_hl_ptr(7, cycles);
      break;
    case BIT_7_A:  // BIT 7, A
      bit_a_r(7);
      break;
    case RES_0_B:  // RES 0, B
      res_b_r(0);
      break;
    case RES_0_C:  // RES 0, C
      res_c_r(0);
      break;
    case RES_0_D:  // RES 0, D
      res_d_r(0);
      break;
    case RES_0_E:  // RES 0, E
      res_e_r(0);
      break;
    case RES_0_H:  // RES 0, H
      res_h_r(0);
      break;
    case RES_0_L:  // RES 0, L
      res_l_r(0);
      break;
    case RES_0_HL:  // RES 0, (HL)
      res_hl_ptr(0, cycles);
      break;
    case RES_0_A:  // RES 0, A
      res_a_r(0);
      break;
    case RES_1_B:  // RES 1, B
      res_b_r(1);
      break;
    case RES_1_C:  // RES 1, C
      res_c_r(1);
      break;
    case RES_1_D:  // RES 1, D
      res_d_r(1);
      break;
    case RES_1_E:  // RES 1, E
      res_e_r(1);
      break;
    case RES_1_H:  // RES 1, H
      res_h_r(1);
      break;
    case RES_1_L:  // RES 1, L
      res_l_r(1);
      break;
    case RES_1_HL:  // RES 1, (HL)
      res_hl_ptr(1, cycles);
      break;
    case RES_1_A:  // RES 1, A
      res_a_r(1);
      break;
    case RES_2_B:  // RES 2, B
      res_b_r(2);
      break;
    case RES_2_C:  // RES 2, C
      res_c_r(2);
      break;
    case RES_2_D:  // RES 2, D
      res_d_r(2);
      break;
    case RES_2_E:  // RES 2, E
      res_e_r(2);
      break;
    case RES_2_H:  // RES 2, H
      res_h_r(2);
      break;
    case RES_2_L:  // RES 2, L
      res_l_r(2);
      break;
    case RES_2_HL:  // RES 2, (HL)
      res_hl_ptr(2, cycles);
      break;
    case RES_2_A:  // RES 2, A
      res_a_r(2);
      break;
    case RES_3_B:  // RES 3, B
      res_b_r(3);
      break;
    case RES_3_C:  // RES 3, C
      res_c_r(3);
      break;
    case RES_3_D:  // RES 3, D
      res_d_r(3);
      break;
    case RES_3_E:  // RES 3, E
      res_e_r(3);
      break;
    case RES_3_H:  // RES 3, H
      res_h_r(3);
      break;
    case RES_3_L:  // RES 3, L
      res_l_r(3);
      break;
    case RES_3_HL:  // RES 3, (HL)
      res_hl_ptr(3, cycles);
      break;
    case RES_3_A:  // RES 3, A
      res_a_r(3);
      break;
    case RES_4_B:  // RES 4, B
      res_b_r(4);
      break;
    case RES_4_C:  // RES 4, C
      res_c_r(4);
      break;
    case RES_4_D:  // RES 4, D
      res_d_r(4);
      break;
    case RES_4_E:  // RES 4, E
      res_e_r(4);
      break;
    case RES_4_H:  // RES 4, H
      res_h_r(4);
      break;
    case RES_4_L:  // RES 4, L
      res_l_r(4);
      break;
    case RES_4_HL:  // RES 4, (HL)
      res_hl_ptr(4, cycles);
      break;
    case RES_4_A:  // RES 4, A
      res_a_r(4);
      break;
    case RES_5_B:  // RES 5, B
      res_b_r(5);
      break;
    case RES_5_C:  // RES 5, C
      res_c_r(5);
      break;
    case RES_5_D:  // RES 5, D
      res_d_r(5);
      break;
    case RES_5_E:  // RES 5, E
      res_e_r(5);
      break;
    case RES_5_H:  // RES 5, H
      res_h_r(5);
      break;
    case RES_5_L:  // RES 5, L
      res_l_r(5);
      break;
    case RES_5_HL:  // RES 5, (HL)
      res_hl_ptr(5, cycles);
      break;
    case RES_5_A:  // RES 5, A
      res_a_r(5);
      break;
    case RES_6_B:  // RES 6, B
      res_b_r(6);
      break;
    case RES_6_C:  // RES 6, C
      res_c_r(6);
      break;
    case RES_6_D:  // RES 6, D
      res_d_r(6);
      break;
    case RES_6_E:  // RES 6, E
      res_e_r(6);
      break;
    case RES_6_H:  // RES 6, H
      res_h_r(6);
      break;
    case RES_6_L:  // RES 6, L
      res_l_r(6);
      break;
    case RES_6_HL:  // RES 6, (HL)
      res_hl_ptr(6, cycles);
      break;
    case RES_6_A:  // RES 6, A
      res_a_r(6);
      break;
    case RES_7_B:  // RES 7, B
      res_b_r(7);
      break;
    case RES_7_C:  // RES 7, C
      res_c_r(7);
      break;
    case RES_7_D:  // RES 7, D
      res_d_r(7);
      break;
    case RES_7_E:  // RES 7, E
      res_e_r(7);
      break;
    case RES_7_H:  // RES 7, H
      res_h_r(7);
      break;
    case RES_7_L:  // RES 7, L
      res_l_r(7);
      break;
    case RES_7_HL:  // RES 7, (HL)
      res_hl_ptr(7, cycles);
      break;
    case RES_7_A:  // RES 7, A
      res_a_r(7);
      break;
    case SET_0_B:  // SET 0, B
      set_b_r(0);
      break;
    case SET_0_C:  // SET 0, C
      set_c_r(0);
      break;
    case SET_0_D:  // SET 0, D
      set_d_r(0);
      break;
    case SET_0_E:  // SET 0, E
      set_e_r(0);
      break;
    case SET_0_H:  // SET 0, H
      set_h_r(0);
      break;
    case SET_0_L:  // SET 0, L
      set_l_r(0);
      break;
    case SET_0_HL:  // SET 0, (HL)
      set_hl_ptr(0, cycles);
      break;
    case SET_0_A:  // SET 0, A
      set_a_r(0);
      break;
    case SET_1_B:  // SET 1, B
      set_b_r(1);
      break;
    case SET_1_C:  // SET 1, C
      set_c_r(1);
      break;
    case SET_1_D:  // SET 1, D
      set_d_r(1);
      break;
    case SET_1_E:  // SET 1, E
      set_e_r(1);
      break;
    case SET_1_H:  // SET 1, H
      set_h_r(1);
      break;
    case SET_1_L:  // SET 1, L
      set_l_r(1);
      break;
    case SET_1_HL:  // SET 1, (HL)
      set_hl_ptr(1, cycles);
      break;
    case SET_1_A:  // SET 1, A
      set_a_r(1);
      break;
    case SET_2_B:  // SET 2, B
      set_b_r(2);
      break;
    case SET_2_C:  // SET 2, C
      set_c_r(2);
      break;
    case SET_2_D:  // SET 2, D
      set_d_r(2);
      break;
    case SET_2_E:  // SET 2, E
      set_e_r(2);
      break;
    case SET_2_H:  // SET 2, H
      set_h_r(2);
      break;
    case SET_2_L:  // SET 2, L
      set_l_r(2);
      break;
    case SET_2_HL:  // SET 2, (HL)
      set_hl_ptr(2, cycles);
      break;
    case SET_2_A:  // SET 2, A
      set_a_r(2);
      break;
    case SET_3_B:  // SET 3, B
      set_b_r(3);
      break;
    case SET_3_C:  // SET 3, C
      set_c_r(3);
      break;
    case SET_3_D:  // SET 3, D
      set_d_r(3);
      break;
    case SET_3_E:  // SET 3, E
      set_e_r(3);
      break;
    case SET_3_H:  // SET 3, H
      set_h_r(3);
      break;
    case SET_3_L:  // SET 3, L
      set_l_r(3);
      break;
    case SET_3_HL:  // SET 3, (HL)
      set_hl_ptr(3, cycles);
      break;
    case SET_3_A:  // SET 3, A
      set_a_r(3);
      break;
    case SET_4_B:  // SET 4, B
      set_b_r(4);
      break;
    case SET_4_C:  // SET 4, C
      set_c_r(4);
      break;
    case SET_4_D:  // SET 4, D
      set_d_r(4);
      break;
    case SET_4_E:  // SET 4, E
      set_e_r(4);
      break;
    case SET_4_H:  // SET 4, H
      set_h_r(4);
      break;
    case SET_4_L:  // SET 4, L
      set_l_r(4);
      break;
    case SET_4_HL:  // SET 4, (HL)
      set_hl_ptr(4, cycles);
      break;
    case SET_4_A:  // SET 4, A
      set_a_r(4);
      break;
    case SET_5_B:  // SET 5, B
      set_b_r(5);
      break;
    case SET_5_C:  //	SET	5, C
      set_c_r(5);
      break;
    case SET_5_D:  // SET 5, D
      set_d_r(5);
      break;
    case SET_5_E:  // SET 5, E
      set_e_r(5);
      break;
    case SET_5_H:  // SET 5, H
      set_h_r(5);
      break;
    case SET_5_L:  // SET 5, L
      set_l_r(5);
      break;
    case SET_5_HL:  // SET 5, (HL)
      set_hl_ptr(5, cycles);
      break;
    case SET_5_A:  // SET 5, A
      set_a_r(5);
      break;
    case SET_6_B:  // SET 6, B
      set_b_r(6);
      break;
    case SET_6_C:  // SET 6, C
      set_c_r(6);
      break;
    case SET_6_D:  // SET 6, D
      set_d_r(6);
      break;
    case SET_6_E:  // SET 6, E
      set_e_r(6);
      break;
    case SET_6_H:  // SET 6, H
      set_h_r(6);
      break;
    case SET_6_L:  // SET 6, L
      set_l_r(6);
      break;
    case SET_6_HL:  // SET 6, (HL)
      set_hl_ptr(6, cycles);
      break;
    case SET_6_A:  // SET 6, A
      set_a_r(6);
      break;
    case SET_7_B:  // SET 7, B
      set_b_r(7);
      break;
    case SET_7_C:  // SET 7, C
      set_c_r(7);
      break;
    case SET_7_D:  // SET 7, D
      set_d_r(7);
      break;
    case SET_7_E:  // SET 7, E
      set_e_r(7);
      break;
    case SET_7_H:  // SET 7, H
      set_h_r(7);
      break;
    case SET_7_L:  // SET 7, L
      set_l_r(7);
      break;
    case SET_7_HL:  // SET 7, (HL)
      set_hl_ptr(7, cycles);
      break;
    case SET_7_A:  // SET 7, A
      set_a_r(7);
      break;
    default:
      break;
  }
}
