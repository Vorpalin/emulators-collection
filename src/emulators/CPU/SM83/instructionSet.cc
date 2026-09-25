#include "SM83.hh"

void SM83::ld_bc_d16(uint32_t &cycles) {
  // Load 16-bit immediate value into BC register pair
  uint8_t lowByte = readMemory(cycles, PC);
  PC++;
  uint8_t highByte = readMemory(cycles, PC);
  PC++;

  B = highByte;
  C = lowByte;
}

void SM83::ld_bc_a(uint32_t &cycles) {
  uint16_t address = (B << 8) | C;  // Combine B and C to form the address
  writeMemory(cycles, address, A);
}

void SM83::inc_bc(uint32_t &cycles) {
  uint16_t bc = (B << 8) | C;  // Combine B and C to form the BC register pair
  bc++;                        // Increment BC
  B = (bc >> 8) & 0xFF;        // Update B with the high byte
  C = bc & 0xFF;               // Update C with the low byte
  ++cycles;                    // Increment cycles for the operation
}

void SM83::inc_b() {
  B++;  // Increment register B
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (B == 0 ? 0x80 : 0) | ((B & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_b() {
  B--;  // Decrement register B
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (B == 0 ? 0x80 : 0) | ((B & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_b_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register B
  B = readMemory(cycles, PC);
  PC++;
}

void SM83::rlca() {
  // Rotate A left through carry
  uint8_t carry = (A & 0x80) >> 7;  // Get the most significant bit (MSB)
  A = (A << 1) | carry;  // Shift A left and set the least significant bit (LSB)
                         // to the carry
  F = (F & 0x10) |
      (carry ? 0x10 : 0);  // Update flags: set carry flag if MSB was 1
}

void SM83::ld_a16_sp(uint32_t &cycles) {
  // Load 16-bit immediate value into SP register
  uint8_t lowByte = readMemory(cycles, PC);
  PC++;
  uint8_t highByte = readMemory(cycles, PC);
  PC++;

  uint16_t address = (highByte << 8) |
                     lowByte;  // Combine high and low bytes to form the address
  ++cycles;                    // Increment cycles for the operation
  writeMemory(cycles, address, SP);
}

void SM83::add_hl_bc(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  uint16_t bc = (B << 8) | C;  // Combine B and C to form the BC register pair

  uint32_t result = hl + bc;  // Perform addition
  H = (result >> 8) & 0xFF;   // Update H with the high byte of the result
  L = result & 0xFF;          // Update L with the low byte of the result

  // Update flags based on the result of the addition
  F = (F & 0x10) |
      (result > 0xFFFF ? 0x10 : 0);  // Set carry flag if overflow occurred
  ++cycles;                          // Increment cycles for the operation
}

void SM83::ld_a_bc(uint32_t &cycles) {
  uint16_t address = (B << 8) | C;  // Combine B and C to form the address
  A = readMemory(cycles,
                 address);  // Load the value from memory into register A
}

void SM83::dec_bc(uint32_t &cycles) {
  uint16_t bc = (B << 8) | C;  // Combine B and C to form the BC register pair
  bc--;                        // Decrement BC
  B = (bc >> 8) & 0xFF;        // Update B with the high byte
  C = bc & 0xFF;               // Update C with the low byte
  ++cycles;                    // Increment cycles for the operation
}

void SM83::inc_c() {
  C++;  // Increment register C
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (C == 0 ? 0x80 : 0) | ((C & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_c() {
  C--;  // Decrement register C
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (C == 0 ? 0x80 : 0) | ((C & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_c_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register C
  C = readMemory(cycles, PC);
  PC++;
}

void SM83::rrca() {
  // Rotate A right through carry
  uint8_t carry = A & 0x01;     // Get the least significant bit (LSB)
  A = (A >> 1) | (carry << 7);  // Shift A right and set the most significant
                                // bit (MSB) to the carry
  F = (F & 0x10) |
      (carry ? 0x10 : 0);  // Update flags: set carry flag if LSB was 1
}

void SM83::stop() {
  // Stop the CPU until an interrupt occurs
  // This is a placeholder implementation; actual behavior may vary based on the
  // emulator design In a real implementation, you would set a flag to indicate
  // the CPU is stopped
}

void SM83::ld_de_d16(uint32_t &cycles) {
  // Load 16-bit immediate value into DE register pair
  uint8_t lowByte = readMemory(cycles, PC);
  PC++;
  uint8_t highByte = readMemory(cycles, PC);
  PC++;

  D = highByte;
  E = lowByte;
}

void SM83::ld_de_a(uint32_t &cycles) {
  uint16_t address = (D << 8) | E;  // Combine D and E to form the address
  writeMemory(cycles, address, A);
}

void SM83::inc_de(uint32_t &cycles) {
  uint16_t de = (D << 8) | E;  // Combine D and E to form the DE register pair
  de++;                        // Increment DE
  D = (de >> 8) & 0xFF;        // Update D with the high byte
  E = de & 0xFF;               // Update E with the low byte
  ++cycles;                    // Increment cycles for the operation
}

void SM83::inc_d() {
  D++;  // Increment register D
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (D == 0 ? 0x80 : 0) | ((D & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_d() {
  D--;  // Decrement register D
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (D == 0 ? 0x80 : 0) | ((D & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_d_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register D
  D = readMemory(cycles, PC);
  PC++;
}

void SM83::rla() {
  // Rotate A left through carry
  uint8_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint8_t newCarry =
      (A & 0x80) >> 7;   // Get the most significant bit (MSB) of A
  A = (A << 1) | carry;  // Shift A left and set the least significant bit (LSB)
                         // to the previous carry
  F = (F & 0x10) |
      (newCarry ? 0x10 : 0);  // Update flags: set carry flag if MSB was 1
}

void SM83::jr_r8(uint32_t &cycles) {
  // Jump relative by signed 8-bit immediate value
  int8_t offset =
      static_cast<int8_t>(readMemory(cycles, PC));  // Read the signed offset
  PC++;                                             // Increment program counter
  PC += offset;  // Apply the offset to the program counter
  ++cycles;      // Increment cycles for the operation
}

void SM83::add_hl_de(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  uint16_t de = (D << 8) | E;  // Combine D and E to form the DE register pair

  uint32_t result = hl + de;  // Perform addition
  H = (result >> 8) & 0xFF;   // Update H with the high byte of the result
  L = result & 0xFF;          // Update L with the low byte of the result

  // Update flags based on the result of the addition
  F = (F & 0x10) |
      (result > 0xFFFF ? 0x10 : 0);  // Set carry flag if overflow occurred
  ++cycles;                          // Increment cycles for the operation
}

void SM83::ld_a_de(uint32_t &cycles) {
  uint16_t address = (D << 8) | E;  // Combine D and E to form the address
  A = readMemory(cycles,
                 address);  // Load the value from memory into register A
}

void SM83::dec_de(uint32_t &cycles) {
  uint16_t de = (D << 8) | E;  // Combine D and E to form the DE register pair
  de--;                        // Decrement DE
  D = (de >> 8) & 0xFF;        // Update D with the high byte
  E = de & 0xFF;               // Update E with the low byte
  ++cycles;                    // Increment cycles for the operation
}

void SM83::inc_e() {
  E++;  // Increment register E
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (E == 0 ? 0x80 : 0) | ((E & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_e() {
  E--;  // Decrement register E
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (E == 0 ? 0x80 : 0) | ((E & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_e_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register E
  E = readMemory(cycles, PC);
  PC++;
}

void SM83::rra() {
  // Rotate A right through carry
  uint8_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint8_t newCarry = A & 0x01;  // Get the least significant bit (LSB) of A
  A = (A >> 1) | (carry << 7);  // Shift A right and set the most significant
                                // bit (MSB) to the previous carry
  F = (F & 0x10) |
      (newCarry ? 0x10 : 0);  // Update flags: set carry flag if LSB was 1
}

void SM83::jr_nz_r8(uint32_t &cycles) {
  // Jump relative by signed 8-bit immediate value if Z flag is not set
  ++cycles;               // Increment cycles for the comparison operation
  if ((F & 0x80) == 0) {  // Check if Z flag is not set
    int8_t offset =
        static_cast<int8_t>(readMemory(cycles, PC));  // Read the signed offset
    PC++;          // Increment program counter
    PC += offset;  // Apply the offset to the program counter
  } else {
    PC++;  // If Z flag is set, just increment PC to skip the offset byte
  }
}

void SM83::ld_hl_d16(uint32_t &cycles) {
  // Load 16-bit immediate value into HL register pair
  uint8_t lowByte = readMemory(cycles, PC);
  PC++;
  uint8_t highByte = readMemory(cycles, PC);
  PC++;

  H = highByte;
  L = lowByte;
}

void SM83::ld_hli_a(uint32_t &cycles) {
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              A);              // Write the value of A to memory at the address
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  hl++;                        // Increment HL
  H = (hl >> 8) & 0xFF;        // Update H with the high byte
  L = hl & 0xFF;               // Update L with the low byte
}

void SM83::inc_hl(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  hl++;                        // Increment HL
  H = (hl >> 8) & 0xFF;        // Update H with the high byte
  L = hl & 0xFF;               // Update L with the low byte
  ++cycles;                    // Increment cycles for the operation
}

void SM83::inc_h() {
  H++;  // Increment register H
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (H == 0 ? 0x80 : 0) | ((H & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_h() {
  H--;  // Decrement register H
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (H == 0 ? 0x80 : 0) | ((H & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_h_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register H
  H = readMemory(cycles, PC);
  PC++;
}

void SM83::daa() {
  // Decimal Adjust Accumulator
  uint8_t correction = 0;
  bool carry = (F & 0x10) != 0;      // Check if carry flag is set
  bool halfCarry = (F & 0x20) != 0;  // Check if half-carry flag is set

  if ((F & 0x80) == 0) {  // If the last operation was an addition
    if (halfCarry || (A & 0x0F) > 9) {
      correction |= 0x06;  // Add 6 to the lower nibble
    }
    if (carry || A > 0x99) {
      correction |= 0x60;  // Add 6 to the upper nibble
      F |= 0x10;           // Set carry flag
    }
  } else {  // If the last operation was a subtraction
    if (halfCarry) {
      correction |= 0x06;  // Subtract 6 from the lower nibble
    }
    if (carry) {
      correction |= 0x60;  // Subtract 6 from the upper nibble
    }
  }

  A += correction;  // Adjust the accumulator
  F = (F & 0x10) |
      (A == 0 ? 0x80 : 0);  // Update flags: set zero flag if A is zero
}

void SM83::jr_z_r8(uint32_t &cycles) {
  // Jump relative by signed 8-bit immediate value if Z flag is set
  ++cycles;               // Increment cycles for the comparison operation
  if ((F & 0x80) != 0) {  // Check if Z flag is set
    int8_t offset =
        static_cast<int8_t>(readMemory(cycles, PC));  // Read the signed offset
    PC++;          // Increment program counter
    PC += offset;  // Apply the offset to the program counter
  } else {
    PC++;  // If Z flag is not set, just increment PC to skip the offset byte
  }
}

void SM83::add_hl_hl(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  uint32_t result = hl + hl;   // Perform addition (HL + HL)
  H = (result >> 8) & 0xFF;    // Update H with the high byte of the result
  L = result & 0xFF;           // Update L with the low byte of the result

  // Update flags based on the result of the addition
  F = (F & 0x10) |
      (result > 0xFFFF ? 0x10 : 0);  // Set carry flag if overflow occurred
  ++cycles;                          // Increment cycles for the operation
}

void SM83::ld_a_hli(uint32_t &cycles) {
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  A = readMemory(cycles,
                 address);     // Load the value from memory into register A
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  hl++;                        // Increment HL
  H = (hl >> 8) & 0xFF;        // Update H with the high byte
  L = hl & 0xFF;               // Update L with the low byte
}

void SM83::dec_hl(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  hl--;                        // Decrement HL
  H = (hl >> 8) & 0xFF;        // Update H with the high byte
  L = hl & 0xFF;               // Update L with the low byte
  ++cycles;                    // Increment cycles for the operation
}

void SM83::inc_l() {
  L++;  // Increment register L
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (L == 0 ? 0x80 : 0) | ((L & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_l() {
  L--;  // Decrement register L
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (L == 0 ? 0x80 : 0) | ((L & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_l_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register L
  L = readMemory(cycles, PC);
  PC++;
}

void SM83::cpl() {
  // Complement (invert) all bits in register A
  A = ~A;     // Invert all bits of A
  F |= 0x20;  // Set the half-carry flag
  F |= 0x10;  // Set the carry flag
}

void SM83::jr_nc_r8(uint32_t &cycles) {
  // Jump relative by signed 8-bit immediate value if C flag is not set
  ++cycles;               // Increment cycles for the comparison operation
  if ((F & 0x10) == 0) {  // Check if C flag is not set
    int8_t offset =
        static_cast<int8_t>(readMemory(cycles, PC));  // Read the signed offset
    PC++;          // Increment program counter
    PC += offset;  // Apply the offset to the program counter
  } else {
    PC++;  // If C flag is set, just increment PC to skip the offset byte
  }
}

void SM83::ld_sp_d16(uint32_t &cycles) {
  // Load 16-bit immediate value into SP register
  uint8_t lowByte = readMemory(cycles, PC);
  PC++;
  uint8_t highByte = readMemory(cycles, PC);
  PC++;

  SP = (highByte << 8) |
       lowByte;  // Combine high and low bytes to form the stack pointer value
}

void SM83::ld_hl_ptr_minus_a(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, hl, A);  // Write the value of A to memory at the address
  hl--;                        // Decrement HL
  H = (hl >> 8) & 0xFF;        // Update H with the high byte
  L = hl & 0xFF;               // Update L with the low byte
}

void SM83::inc_sp(uint32_t &cycles) {
  SP++;      // Increment stack pointer
  ++cycles;  // Increment cycles for the operation
}

void SM83::inc_hl_ptr(uint32_t &cycles) {
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  value++;                                      // Increment the value
  writeMemory(cycles, address,
              value);  // Write the incremented value back to memory
  F = (F & 0x10) | (value == 0 ? 0x80 : 0) |
      ((value & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::dec_hl_ptr(uint32_t &cycles) {
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  value--;                                      // Decrement the value
  writeMemory(cycles, address,
              value);  // Write the decremented value back to memory
  F = (F & 0x10) | (value == 0 ? 0x80 : 0) |
      ((value & 0x0F) == 0x0F ? 0x20 : 0);  // Update flags based on the result
}

void SM83::ld_hl_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value =
      readMemory(cycles, PC);  // Read the immediate value from memory
  PC++;                        // Increment program counter
  writeMemory(cycles, address,
              value);  // Write the value to memory at address HL
}

void SM83::scf() {
  // Set carry flag
  F |= 0x10;   // Set the carry flag
  F &= ~0x20;  // Clear the half-carry flag
  F &= ~0x80;  // Clear the zero flag
}

void SM83::jr_c_r8(uint32_t &cycles) {
  // Jump relative by signed 8-bit immediate value if C flag is set
  ++cycles;               // Increment cycles for the comparison operation
  if ((F & 0x10) != 0) {  // Check if C flag is set
    int8_t offset =
        static_cast<int8_t>(readMemory(cycles, PC));  // Read the signed offset
    PC++;          // Increment program counter
    PC += offset;  // Apply the offset to the program counter
  } else {
    PC++;  // If C flag is not set, just increment PC to skip the offset byte
  }
}

void SM83::add_hl_sp(uint32_t &cycles) {
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  uint32_t result = hl + SP;   // Perform addition (HL + SP)
  H = (result >> 8) & 0xFF;    // Update H with the high byte of the result
  L = result & 0xFF;           // Update L with the low byte of the result

  // Update flags based on the result of the addition
  F = (F & 0x10) |
      (result > 0xFFFF ? 0x10 : 0);  // Set carry flag if overflow occurred
  ++cycles;                          // Increment cycles for the operation
}

void SM83::ld_a_hld(uint32_t &cycles) {
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  A = readMemory(cycles,
                 address);     // Load the value from memory into register A
  uint16_t hl = (H << 8) | L;  // Combine H and L to form the HL register pair
  hl--;                        // Decrement HL
  H = (hl >> 8) & 0xFF;        // Update H with the high byte
  L = hl & 0xFF;               // Update L with the low byte
}

void SM83::dec_sp(uint32_t &cycles) {
  SP--;      // Decrement stack pointer
  ++cycles;  // Increment cycles for the operation
}

void SM83::inc_a() {
  A++;  // Increment register A
  // Update flags based on the result of the increment operation
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) | ((A & 0x0F) == 0 ? 0x20 : 0);
}

void SM83::dec_a() {
  A--;  // Decrement register A
  // Update flags based on the result of the decrement operation
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) | ((A & 0x0F) == 0x0F ? 0x20 : 0);
}

void SM83::ld_a_d8(uint32_t &cycles) {
  // Load 8-bit immediate value into register A
  A = readMemory(cycles, PC);
  PC++;
}

void SM83::ccf() {
  // Complement carry flag
  F ^= 0x10;   // Toggle the carry flag
  F &= ~0x20;  // Clear the half-carry flag
  F &= ~0x80;  // Clear the zero flag
}
