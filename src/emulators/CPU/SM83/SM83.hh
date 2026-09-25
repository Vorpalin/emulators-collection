#pragma once

#include <cstdint>

#include "Bus.hh"
#include "CPU.hh"

class SM83 : CPU {
 public:
  SM83(Bus *bus);
  ~SM83();

  void reset() override;
  uint32_t execute() override;
  uint8_t readMemory(uint32_t &cycles, uint16_t address) override;
  void writeMemory(uint32_t &cycles, uint16_t address, uint8_t value) override;

 private:
  uint8_t A, B, C, D, E, H, L;
  uint16_t PC, SP;
  uint8_t F;

  Bus *bus;

  // Instruction set methods
  void ld_bc_d16(uint32_t &cycles);
  void ld_bc_a(uint32_t &cycles);
  void inc_bc(uint32_t &cycles);
  void inc_b();
  void dec_b();
  void ld_b_d8(uint32_t &cycles);
  void rlca();
  void ld_a16_sp(uint32_t &cycles);
  void add_hl_bc(uint32_t &cycles);
  void ld_a_bc(uint32_t &cycles);
  void dec_bc(uint32_t &cycles);
  void inc_c();
  void dec_c();
  void ld_c_d8(uint32_t &cycles);
  void rrca();
  void stop();
  void ld_de_d16(uint32_t &cycles);
  void ld_de_a(uint32_t &cycles);
  void inc_de(uint32_t &cycles);
  void inc_d();
  void dec_d();
  void ld_d_d8(uint32_t &cycles);
  void rla();
  void jr_r8(uint32_t &cycles);
  void add_hl_de(uint32_t &cycles);
  void ld_a_de(uint32_t &cycles);
  void dec_de(uint32_t &cycles);
  void inc_e();
  void dec_e();
  void ld_e_d8(uint32_t &cycles);
  void rra();
  void jr_nz_r8(uint32_t &cycles);
  void ld_hl_d16(uint32_t &cycles);
  void ld_hli_a(uint32_t &cycles);
  void inc_hl(uint32_t &cycles);
  void inc_h();
  void dec_h();
  void ld_h_d8(uint32_t &cycles);
  void daa();
  void jr_z_r8(uint32_t &cycles);
  void add_hl_hl(uint32_t &cycles);
  void ld_a_hli(uint32_t &cycles);
  void dec_hl(uint32_t &cycles);
  void inc_l();
  void dec_l();
  void ld_l_d8(uint32_t &cycles);
  void cpl();
  void jr_nc_r8(uint32_t &cycles);
  void ld_sp_d16(uint32_t &cycles);
  void ld_hl_ptr_minus_a(uint32_t &cycles);
  void inc_sp(uint32_t &cycles);
  void inc_hl_ptr(uint32_t &cycles);
  void dec_hl_ptr(uint32_t &cycles);
  void ld_hl_d8(uint32_t &cycles);
  void scf();
  void jr_c_r8(uint32_t &cycles);
  void add_hl_sp(uint32_t &cycles);
  void ld_a_hld(uint32_t &cycles);
  void dec_sp(uint32_t &cycles);
  void inc_a();
  void dec_a();
  void ld_a_d8(uint32_t &cycles);
  void ccf();
};
