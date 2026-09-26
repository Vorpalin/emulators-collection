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

void SM83::ld_b_b() {
  // Load the value of register B into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_b_c() {
  // Load the value of register C into register B
  B = C;
}

void SM83::ld_b_d() {
  // Load the value of register D into register B
  B = D;
}

void SM83::ld_b_e() {
  // Load the value of register E into register B
  B = E;
}

void SM83::ld_b_h() {
  // Load the value of register H into register B
  B = H;
}

void SM83::ld_b_l() {
  // Load the value of register L into register B
  B = L;
}

void SM83::ld_b_a() {
  // Load the value of register A into register B
  B = A;
}

void SM83::ld_b_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register B
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  B = readMemory(cycles, address);  // Read the value from memory into B
}

void SM83::ld_c_b() {
  // Load the value of register B into register C
  C = B;
}

void SM83::ld_c_c() {
  // Load the value of register C into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_c_d() {
  // Load the value of register D into register C
  C = D;
}

void SM83::ld_c_e() {
  // Load the value of register E into register C
  C = E;
}

void SM83::ld_c_h() {
  // Load the value of register H into register C
  C = H;
}

void SM83::ld_c_l() {
  // Load the value of register L into register C
  C = L;
}

void SM83::ld_c_a() {
  // Load the value of register A into register C
  C = A;
}

void SM83::ld_c_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register C
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  C = readMemory(cycles, address);  // Read the value from memory into C
}

void SM83::ld_d_b() {
  // Load the value of register B into register D
  D = B;
}

void SM83::ld_d_c() {
  // Load the value of register C into register D
  D = C;
}

void SM83::ld_d_d() {
  // Load the value of register D into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_d_e() {
  // Load the value of register E into register D
  D = E;
}

void SM83::ld_d_h() {
  // Load the value of register H into register D
  D = H;
}

void SM83::ld_d_l() {
  // Load the value of register L into register D
  D = L;
}

void SM83::ld_d_a() {
  // Load the value of register A into register D
  D = A;
}

void SM83::ld_d_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register D
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  D = readMemory(cycles, address);  // Read the value from memory into D
}

void SM83::ld_e_b() {
  // Load the value of register B into register E
  E = B;
}

void SM83::ld_e_c() {
  // Load the value of register C into register E
  E = C;
}

void SM83::ld_e_d() {
  // Load the value of register D into register E
  E = D;
}

void SM83::ld_e_e() {
  // Load the value of register E into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_e_h() {
  // Load the value of register H into register E
  E = H;
}

void SM83::ld_e_l() {
  // Load the value of register L into register E
  E = L;
}

void SM83::ld_e_a() {
  // Load the value of register A into register E
  E = A;
}

void SM83::ld_e_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register E
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  E = readMemory(cycles, address);  // Read the value from memory into E
}

void SM83::ld_h_b() {
  // Load the value of register B into register H
  H = B;
}

void SM83::ld_h_c() {
  // Load the value of register C into register H
  H = C;
}

void SM83::ld_h_d() {
  // Load the value of register D into register H
  H = D;
}

void SM83::ld_h_e() {
  // Load the value of register E into register H
  H = E;
}

void SM83::ld_h_h() {
  // Load the value of register H into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_h_l() {
  // Load the value of register L into register H
  H = L;
}

void SM83::ld_h_a() {
  // Load the value of register A into register H
  H = A;
}

void SM83::ld_h_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register H
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  H = readMemory(cycles, address);  // Read the value from memory into H
}

void SM83::ld_l_b() {
  // Load the value of register B into register L
  L = B;
}

void SM83::ld_l_c() {
  // Load the value of register C into register L
  L = C;
}

void SM83::ld_l_d() {
  // Load the value of register D into register L
  L = D;
}

void SM83::ld_l_e() {
  // Load the value of register E into register L
  L = E;
}

void SM83::ld_l_h() {
  // Load the value of register H into register L
  L = H;
}

void SM83::ld_l_l() {
  // Load the value of register L into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_l_a() {
  // Load the value of register A into register L
  L = A;
}

void SM83::ld_l_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register L
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  L = readMemory(cycles, address);  // Read the value from memory into L
}

void SM83::ld_hl_b(uint32_t &cycles) {
  // Load the value of register B into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              B);  // Write the value of B to memory at the address
}

void SM83::ld_hl_c(uint32_t &cycles) {
  // Load the value of register C into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              C);  // Write the value of C to memory at the address
}

void SM83::ld_hl_d(uint32_t &cycles) {
  // Load the value of register D into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              D);  // Write the value of D to memory at the address
}

void SM83::ld_hl_e(uint32_t &cycles) {
  // Load the value of register E into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              E);  // Write the value of E to memory at the address
}

void SM83::ld_hl_h(uint32_t &cycles) {
  // Load the value of register H into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              H);  // Write the value of H to memory at the address
}

void SM83::ld_hl_l(uint32_t &cycles) {
  // Load the value of register L into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              L);  // Write the value of L to memory at the address
}

void SM83::ld_hl_a(uint32_t &cycles) {
  // Load the value of register A into memory at address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  writeMemory(cycles, address,
              A);  // Write the value of A to memory at the address
}

void SM83::halt() {
  // Halt the CPU until an interrupt occurs
  // This is a placeholder implementation; actual behavior may vary based on the
  // emulator design In a real implementation, you would set a flag to indicate
  // the CPU is halted
}

void SM83::ld_a_b() {
  // Load the value of register B into register A
  A = B;
}

void SM83::ld_a_c() {
  // Load the value of register C into register A
  A = C;
}

void SM83::ld_a_d() {
  // Load the value of register D into register A
  A = D;
}

void SM83::ld_a_e() {
  // Load the value of register E into register A
  A = E;
}

void SM83::ld_a_h() {
  // Load the value of register H into register A
  A = H;
}

void SM83::ld_a_l() {
  // Load the value of register L into register A
  A = L;
}

void SM83::ld_a_a() {
  // Load the value of register A into itself (no operation)
  // This is a placeholder implementation; in a real implementation, you might
  // want to update flags or perform other operations as needed
}

void SM83::ld_a_hl_ptr(uint32_t &cycles) {
  // Load the value from memory at address HL into register A
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  A = readMemory(cycles, address);  // Read the value from memory into A
}

void SM83::add_a_b() {
  // Add the value of register B to register A
  uint16_t result = A + B;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_c() {
  // Add the value of register C to register A
  uint16_t result = A + C;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_d() {
  // Add the value of register D to register A
  uint16_t result = A + D;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_e() {
  // Add the value of register E to register A
  uint16_t result = A + E;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_h() {
  // Add the value of register H to register A
  uint16_t result = A + H;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_l() {
  // Add the value of register L to register A
  uint16_t result = A + L;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_a() {
  // Add the value of register A to itself
  uint16_t result = A + A;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::add_a_hl_ptr(uint32_t &cycles) {
  // Add the value from memory at address HL to register A
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint16_t result = A + value;                  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_b() {
  // Add the value of register B and the carry flag to register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + B + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_c() {
  // Add the value of register C and the carry flag to register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + C + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_d() {
  // Add the value of register D and the carry flag to register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + D + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_e() {
  // Add the value of register E and the carry flag to register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + E + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_h() {
  // Add the value of register H and the carry flag to register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + H + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_l() {
  // Add the value of register L and the carry flag to register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + L + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_a() {
  // Add the value of register A and the carry flag to itself
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + A + carry;      // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::adc_a_hl_ptr(uint32_t &cycles) {
  // Add the value from memory at address HL and the carry flag to register A
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint16_t carry = (F & 0x10) ? 1 : 0;          // Get the current carry flag
  uint16_t result = A + value + carry;          // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_b() {
  // Subtract the value of register B from register A
  uint16_t result = A - B;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_c() {
  // Subtract the value of register C from register A
  uint16_t result = A - C;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_d() {
  // Subtract the value of register D from register A
  uint16_t result = A - D;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_e() {
  // Subtract the value of register E from register A
  uint16_t result = A - E;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_h() {
  // Subtract the value of register H from register A
  uint16_t result = A - H;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_l() {
  // Subtract the value of register L from register A
  uint16_t result = A - L;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_a() {
  // Subtract the value of register A from itself
  uint16_t result = A - A;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sub_a_hl_ptr(uint32_t &cycles) {
  // Subtract the value from memory at address HL from register A
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint16_t result = A - value;                  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_b() {
  // Subtract the value of register B and the carry flag from register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - B - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_c() {
  // Subtract the value of register C and the carry flag from register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - C - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_d() {
  // Subtract the value of register D and the carry flag from register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - D - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_e() {
  // Subtract the value of register E and the carry flag from register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - E - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_h() {
  // Subtract the value of register H and the carry flag from register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - H - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_l() {
  // Subtract the value of register L and the carry flag from register A
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - L - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_a() {
  // Subtract the value of register A and the carry flag from itself
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - A - carry;      // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::sbc_a_hl_ptr(uint32_t &cycles) {
  // Subtract the value from memory at address HL and the carry flag from
  // register A
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint16_t carry = (F & 0x10) ? 1 : 0;          // Get the current carry flag
  uint16_t result = A - value - carry;  // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::and_a_b() {
  // Perform bitwise AND between register A and register B
  A &= B;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_c() {
  // Perform bitwise AND between register A and register C
  A &= C;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_d() {
  // Perform bitwise AND between register A and register D
  A &= D;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_e() {
  // Perform bitwise AND between register A and register E
  A &= E;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_h() {
  // Perform bitwise AND between register A and register H
  A &= H;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_l() {
  // Perform bitwise AND between register A and register L
  A &= L;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_a() {
  // Perform bitwise AND between register A and itself
  A &= A;  // Update register A with the result (no change)
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::and_a_hl_ptr(uint32_t &cycles) {
  // Perform bitwise AND between register A and the value from memory at address
  // HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  A &= value;  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::or_a_b() {
  // Perform bitwise OR between register A and register B
  A |= B;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_c() {
  // Perform bitwise OR between register A and register C
  A |= C;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_d() {
  // Perform bitwise OR between register A and register D
  A |= D;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_e() {
  // Perform bitwise OR between register A and register E
  A |= E;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_h() {
  // Perform bitwise OR between register A and register H
  A |= H;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_l() {
  // Perform bitwise OR between register A and register L
  A |= L;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_a() {
  // Perform bitwise OR between register A and itself
  A |= A;  // Update register A with the result (no change)
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::or_a_hl_ptr(uint32_t &cycles) {
  // Perform bitwise OR between register A and the value from memory at address
  // HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  A |= value;                            // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_b() {
  // Perform bitwise XOR between register A and register B
  A ^= B;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_c() {
  // Perform bitwise XOR between register A and register C
  A ^= C;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_d() {
  // Perform bitwise XOR between register A and register D
  A ^= D;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_e() {
  // Perform bitwise XOR between register A and register E
  A ^= E;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_h() {
  // Perform bitwise XOR between register A and register H
  A ^= H;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_l() {
  // Perform bitwise XOR between register A and register L
  A ^= L;                                // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_a() {
  // Perform bitwise XOR between register A and itself
  A ^= A;  // Update register A with the result (will be 0)
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::xor_a_hl_ptr(uint32_t &cycles) {
  // Perform bitwise XOR between register A and the value from memory at address
  // HL
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  A ^= value;                            // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_b() {
  // Compare register A with register B (A - B)
  uint16_t result = A - B;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_c() {
  // Compare register A with register C (A - C)
  uint16_t result = A - C;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_d() {
  // Compare register A with register D (A - D)
  uint16_t result = A - D;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_e() {
  // Compare register A with register E (A - E)
  uint16_t result = A - E;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_h() {
  // Compare register A with register H (A - H)
  uint16_t result = A - H;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_l() {
  // Compare register A with register L (A - L)
  uint16_t result = A - L;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_a() {
  // Compare register A with itself (A - A)
  uint16_t result = A - A;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::cp_a_hl_ptr(uint32_t &cycles) {
  // Compare register A with the value from memory at address HL (A - [HL])
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint16_t result = A - value;                  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::ret_nz(uint32_t &cycles) {
  // Return from subroutine if the zero flag is not set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x80) == 0) {  // Check if the zero flag is not set
    uint16_t address =
        popStack(cycles);  // Pop the return address from the stack
    PC = address;          // Set the program counter to the return address
    ++cycles;              // Increment cycles for the return operation
  }
}

void SM83::pop_bc(uint32_t &cycles) {
  // Pop two bytes from the stack into registers B and C
  C = readMemory(cycles, SP);  // Pop the low byte into register C
  SP++;                        // Increment stack pointer
  B = readMemory(cycles, SP);  // Pop the high byte into register B
  SP++;                        // Increment stack pointer
}

void SM83::jp_nz_a16(uint32_t &cycles) {
  // Jump to the address specified by the next two bytes if the zero flag is not
  // set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x80) == 0) {  // Check if the zero flag is not set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    PC = address;  // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not jumping
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::jp_a16(uint32_t &cycles) {
  // Jump to the address specified by the next two bytes
  uint16_t address =
      readMemory(cycles, PC);                // Read the low byte of the address
  PC++;                                      // Increment program counter
  address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
  PC++;                                      // Increment program counter
  PC = address;  // Set the program counter to the new address
  cycles += 1;   // Increment cycles for the jump operation
}

void SM83::call_nz_a16(uint32_t &cycles) {
  // Call the subroutine at the address specified by the next two bytes if the
  // zero flag is not set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x80) == 0) {  // Check if the zero flag is not set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    writeMemory(cycles, SP,
                PC);  // Push the current program counter onto the stack
    PC = address;     // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not calling
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::add_a_d8(uint32_t &cycles) {
  // Add the immediate 8-bit value to register A
  uint8_t value =
      readMemory(cycles, PC);   // Read the immediate value from memory
  PC++;                         // Increment program counter
  uint16_t result = A + value;  // Perform addition
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::rst_00(uint32_t &cycles) {
  // Restart the program at address 0x00
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x00;              // Set the program counter to address 0x00
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::ret_z(uint32_t &cycles) {
  // Return from subroutine if the zero flag is set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x80) != 0) {  // Check if the zero flag is set
    uint16_t address =
        popStack(cycles);  // Pop the return address from the stack
    PC = address;          // Set the program counter to the return address
    ++cycles;              // Increment cycles for the return operation
  }
}

void SM83::ret(uint32_t &cycles) {
  // Unconditional return from subroutine
  uint16_t address = popStack(cycles);  // Pop the return address from the stack
  PC = address;  // Set the program counter to the return address
  ++cycles;      // Increment cycles for the return operation
}

void SM83::jp_z_a16(uint32_t &cycles) {
  // Jump to the address specified by the next two bytes if the zero flag is set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x80) != 0) {  // Check if the zero flag is set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    PC = address;  // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not jumping
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::prefix_bc(uint32_t &cycles) {
  // Handle the CB prefix instruction set
  uint8_t opcode =
      readMemory(cycles, PC);  // Read the next opcode after the CB prefix
  PC++;                        // Increment program counter
  executeCBInstruction(
      opcode, cycles);  // Execute the CB instruction based on the opcode
}

void SM83::call_z_a16(uint32_t &cycles) {
  // Call the subroutine at the address specified by the next two bytes if the
  // zero flag is set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x80) != 0) {  // Check if the zero flag is set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    writeMemory(cycles, SP,
                PC);  // Push the current program counter onto the stack
    PC = address;     // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not calling
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::call_a16(uint32_t &cycles) {
  // Call the subroutine at the address specified by the next two bytes
  // unconditionally
  uint16_t address =
      readMemory(cycles, PC);                // Read the low byte of the address
  PC++;                                      // Increment program counter
  address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
  PC++;                                      // Increment program counter
  writeMemory(cycles, SP,
              PC);  // Push the current program counter onto the stack
  PC = address;     // Set the program counter to the new address
}

void SM83::adc_a_d8(uint32_t &cycles) {
  // Add the immediate 8-bit value and the carry flag to register A
  uint8_t value =
      readMemory(cycles, PC);           // Read the immediate value from memory
  PC++;                                 // Increment program counter
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A + value + carry;  // Perform addition with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if overflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::rst_08(uint32_t &cycles) {
  // Restart the program at address 0x08
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x08;              // Set the program counter to address 0x08
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::ret_nc(uint32_t &cycles) {
  // Return from subroutine if the carry flag is not set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x10) == 0) {  // Check if the carry flag is not set
    uint16_t address =
        popStack(cycles);  // Pop the return address from the stack
    PC = address;          // Set the program counter to the return address
    ++cycles;              // Increment cycles for the return operation
  }
}

void SM83::pop_de(uint32_t &cycles) {
  // Pop two bytes from the stack into registers D and E
  E = readMemory(cycles, SP);  // Pop the low byte into register E
  SP++;                        // Increment stack pointer
  D = readMemory(cycles, SP);  // Pop the high byte into register D
  SP++;                        // Increment stack pointer
}

void SM83::jp_nc_a16(uint32_t &cycles) {
  // Jump to the address specified by the next two bytes if the carry flag is
  // not set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x10) == 0) {  // Check if the carry flag is not set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    PC = address;  // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not jumping
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::call_nc_a16(uint32_t &cycles) {
  // Call the subroutine at the address specified by the next two bytes if the
  // carry flag is not set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x10) == 0) {  // Check if the carry flag is not set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    writeMemory(cycles, SP,
                PC);  // Push the current program counter onto the stack
    PC = address;     // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not calling
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::push_de(uint32_t &cycles) {
  // Push the values of registers D and E onto the stack
  writeMemory(cycles, SP - 1, D);  // Write the high byte (D) to the stack
  writeMemory(cycles, SP - 2, E);  // Write the low byte (E) to the stack
  SP -= 2;                         // Decrement stack pointer by 2
  ++cycles;                        // Increment cycles for the push operation
}

void SM83::sub_a_d8(uint32_t &cycles) {
  // Subtract the immediate 8-bit value from register A
  uint8_t value =
      readMemory(cycles, PC);   // Read the immediate value from memory
  PC++;                         // Increment program counter
  uint16_t result = A - value;  // Perform subtraction
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::rst_10(uint32_t &cycles) {
  // Restart the program at address 0x10
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x10;              // Set the program counter to address 0x10
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::ret_c(uint32_t &cycles) {
  // Return from subroutine if the carry flag is set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x10) != 0) {  // Check if the carry flag is set
    uint16_t address =
        popStack(cycles);  // Pop the return address from the stack
    PC = address;          // Set the program counter to the return address
    ++cycles;              // Increment cycles for the return operation
  }
}

void SM83::reti(uint32_t &cycles) {
  // Return from interrupt and enable interrupts
  uint16_t address = popStack(cycles);  // Pop the return address from the stack
  PC = address;  // Set the program counter to the return address
  ++cycles;      // Increment cycles for the return operation
  IME = true;    // Enable interrupts
  ++cycles;      // Increment cycles for the return operation
}

void SM83::jp_c_a16(uint32_t &cycles) {
  // Jump to the address specified by the next two bytes if the carry flag is
  // set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x10) != 0) {  // Check if the carry flag is set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    PC = address;  // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not jumping
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::call_c_a16(uint32_t &cycles) {
  // Call the subroutine at the address specified by the next two bytes if the
  // carry flag is set
  ++cycles;               // Increment cycles for the conditional check
  if ((F & 0x10) != 0) {  // Check if the carry flag is set
    uint16_t address =
        readMemory(cycles, PC);  // Read the low byte of the address
    PC++;                        // Increment program counter
    address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
    PC++;                                      // Increment program counter
    writeMemory(cycles, SP,
                PC);  // Push the current program counter onto the stack
    PC = address;     // Set the program counter to the new address
  } else {
    PC += 2;      // Skip over the two bytes of the address if not calling
    cycles += 1;  // Increment cycles for reading the two bytes
  }
}

void SM83::sbc_a_d8(uint32_t &cycles) {
  // Subtract the immediate 8-bit value and the carry flag from register A
  uint8_t value =
      readMemory(cycles, PC);           // Read the immediate value from memory
  PC++;                                 // Increment program counter
  uint16_t carry = (F & 0x10) ? 1 : 0;  // Get the current carry flag
  uint16_t result = A - value - carry;  // Perform subtraction with carry
  F = (F & 0x10) |
      (result > 0xFF ? 0x10 : 0);  // Update carry flag if underflow occurred
  A = result & 0xFF;               // Store the result in register A
}

void SM83::rst_18(uint32_t &cycles) {
  // Restart the program at address 0x18
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x18;              // Set the program counter to address 0x18
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::ldh_a8_a(uint32_t &cycles) {
  // Load the value of register A into memory at address 0xFF00 + immediate
  // 8-bit value
  uint8_t offset =
      readMemory(cycles, PC);          // Read the immediate offset from memory
  PC++;                                // Increment program counter
  uint16_t address = 0xFF00 + offset;  // Calculate the target address
  writeMemory(cycles, address,
              A);  // Write the value of register A to the target address
}

void SM83::pop_hl(uint32_t &cycles) {
  // Pop two bytes from the stack into registers H and L
  L = readMemory(cycles, SP);  // Pop the low byte into register L
  SP++;                        // Increment stack pointer
  H = readMemory(cycles, SP);  // Pop the high byte into register H
  SP++;                        // Increment stack pointer
}

void SM83::ldh_c_a(uint32_t &cycles) {
  // Load the value of register A into memory at address 0xFF00 + register C
  uint16_t address = 0xFF00 + C;  // Calculate the target address
  writeMemory(cycles, address,
              A);  // Write the value of register A to the target address
}

void SM83::push_hl(uint32_t &cycles) {
  // Push the values of registers H and L onto the stack
  writeMemory(cycles, SP - 1, H);  // Write the high byte (H) to the stack
  writeMemory(cycles, SP - 2, L);  // Write the low byte (L) to the stack
  SP -= 2;                         // Decrement stack pointer by 2
  ++cycles;                        // Increment cycles for the push operation
}

void SM83::and_a_d8(uint32_t &cycles) {
  // Perform bitwise AND between register A and the immediate 8-bit value
  uint8_t value =
      readMemory(cycles, PC);  // Read the immediate value from memory
  PC++;                        // Increment program counter
  A &= value;                  // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0) |
      ((A & 0x0F) == 0 ? 0x20 : 0);  // Update flags based on the result
}

void SM83::rst_20(uint32_t &cycles) {
  // Restart the program at address 0x20
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x20;              // Set the program counter to address 0x20
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::add_sp_r8(uint32_t &cycles) {
  // Add the signed 8-bit value to the stack pointer (SP)
  int8_t value = static_cast<int8_t>(
      readMemory(cycles, PC));   // Read the immediate signed value from memory
  PC++;                          // Increment program counter
  uint16_t result = SP + value;  // Perform addition
  F = (F & 0x10) | ((SP ^ value ^ result) & 0x10
                        ? 0x10
                        : 0);  // Update carry flag if overflow occurred
  SP = result;                 // Store the result in stack pointer
}

void SM83::jp_hl(uint32_t &cycles) {
  // Jump to the address contained in registers H and L
  uint16_t address = (H << 8) | L;  // Combine H and L to form the address
  PC = address;  // Set the program counter to the new address
  ++cycles;      // Increment cycles for the jump operation
}

void SM83::ld_a16_a(uint32_t &cycles) {
  // Load the value of register A into memory at the address specified by the
  // next two bytes
  uint16_t address =
      readMemory(cycles, PC);                // Read the low byte of the address
  PC++;                                      // Increment program counter
  address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
  PC++;                                      // Increment program counter
  writeMemory(cycles, address,
              A);  // Write the value of register A to the target address
}

void SM83::xor_a_d8(uint32_t &cycles) {
  // Perform bitwise XOR between register A and the immediate 8-bit value
  uint8_t value =
      readMemory(cycles, PC);            // Read the immediate value from memory
  PC++;                                  // Increment program counter
  A ^= value;                            // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::rst_28(uint32_t &cycles) {
  // Restart the program at address 0x28
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x28;              // Set the program counter to address 0x28
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::pop_af(uint32_t &cycles) {
  // Pop two bytes from the stack into registers A and F
  F = readMemory(cycles, SP);  // Pop the low byte into register F
  SP++;                        // Increment stack pointer
  A = readMemory(cycles, SP);  // Pop the high byte into register A
  SP++;                        // Increment stack pointer
}

void SM83::ld_a_c(uint32_t &cycles) {
  // Load the value from memory at address 0xFF00 + register C into register A
  uint16_t address = 0xFF00 + C;  // Calculate the target address
  A = readMemory(
      cycles,
      address);  // Read the value from memory and store it in register A
}

void SM83::di() {
  // Disable interrupts
  IME = false;  // Clear the interrupt master enable flag
}

void SM83::push_af(uint32_t &cycles) {
  // Push the values of registers A and F onto the stack
  writeMemory(cycles, SP - 1, A);  // Write the high byte (A) to the stack
  writeMemory(cycles, SP - 2, F);  // Write the low byte (F) to the stack
  SP -= 2;                         // Decrement stack pointer by 2
  ++cycles;                        // Increment cycles for the push operation
}

void SM83::or_a_d8(uint32_t &cycles) {
  // Perform bitwise OR between register A and the immediate 8-bit value
  uint8_t value =
      readMemory(cycles, PC);            // Read the immediate value from memory
  PC++;                                  // Increment program counter
  A |= value;                            // Update register A with the result
  F = (F & 0x10) | (A == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::rst_30(uint32_t &cycles) {
  // Restart the program at address 0x30
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x30;              // Set the program counter to address 0x30
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::ld_hl_sp_plus_r8(uint32_t &cycles) {
  // Load the value of SP + signed 8-bit value into registers H and L
  int8_t offset = static_cast<int8_t>(
      readMemory(cycles, PC));    // Read the immediate signed value from memory
  PC++;                           // Increment program counter
  uint16_t result = SP + offset;  // Perform addition
  ++cycles;                       // Increment cycles for the addition operation
  H = (result >> 8) & 0xFF;       // Store the high byte in register H
  L = result & 0xFF;              // Store the low byte in register L
}

void SM83::ld_sp_hl(uint32_t &cycles) {
  // Load the value of registers H and L into the stack pointer (SP)
  SP = (H << 8) | L;  // Combine H and L to form the new stack pointer value
  ++cycles;           // Increment cycles for the load operation
}

void SM83::ld_a_a16_ptr(uint32_t &cycles) {
  // Load the value from memory at the address specified by the next two bytes
  // into register A
  uint16_t address =
      readMemory(cycles, PC);                // Read the low byte of the address
  PC++;                                      // Increment program counter
  address |= (readMemory(cycles, PC) << 8);  // Read the high byte and combine
  PC++;                                      // Increment program counter
  A = readMemory(
      cycles,
      address);  // Read the value from memory and store it in register A
}

void SM83::ei() {
  // Enable interrupts
  IME = true;  // Set the interrupt master enable flag
}

void SM83::cp_a_d8(uint32_t &cycles) {
  // Compare register A with the immediate 8-bit value (A - d8)
  uint8_t value =
      readMemory(cycles, PC);   // Read the immediate value from memory
  PC++;                         // Increment program counter
  uint16_t result = A - value;  // Perform subtraction
  F = (F & 0x10) | (result > 0xFF ? 0x10 : 0) |
      (result == 0 ? 0x80 : 0);  // Update flags based on the result
}

void SM83::rst_38(uint32_t &cycles) {
  // Restart the program at address 0x38
  pushStack(cycles, PC);  // Push the current program counter onto the stack
  PC = 0x38;              // Set the program counter to address 0x38
  ++cycles;               // Increment cycles for the restart operation
}

void SM83::ldh_a_a8(uint32_t &cycles) {
  // Load the value from memory at address 0xFF00 + immediate 8-bit value into
  // register A
  uint8_t offset =
      readMemory(cycles, PC);          // Read the immediate offset from memory
  PC++;                                // Increment program counter
  uint16_t address = 0xFF00 + offset;  // Calculate the target address
  A = readMemory(
      cycles,
      address);  // Read the value from memory and store it in register A
}
