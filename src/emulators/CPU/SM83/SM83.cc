#include "SM83.hh"

#include "opcodes.hh"

SM83::SM83(Bus *bus) : CPU() {
  // Initialize registers and flags
  A = B = C = D = E = H = L = 0;
  F = 0;
  PC = 0;
  SP = 0xFFFE;  // Stack pointer initialized to top of stack
  this->bus = bus;
}

SM83::~SM83() {
  // Destructor logic if needed
}

void SM83::reset() {
  // Reset registers and flags
  A = B = C = D = E = H = L = 0;
  F = 0;
  PC = 0;
  SP = 0xFFFE;   // Reset stack pointer
  bus->reset();  // Reset the bus if needed
}

uint8_t SM83::readMemory(uint32_t &cycles, uint16_t address) {
  // Read a byte from memory through the bus
  ++cycles;  // Increment cycles for the memory read operation
  return bus->readMemory(address);
}

void SM83::writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) {
  // Write a byte to memory through the bus
  ++cycles;  // Increment cycles for the memory write operation
  bus->writeMemory(address, value);
}

uint32_t SM83::execute() {
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
    default:
      break;
  }

  return cycles;  // Return the number of cycles taken for this instruction
}
