#include "SM83.hh"

void SM83::rlc_b() {
  // Rotate B left through carry
  uint8_t carry = (B & 0x80) >> 7;  // Get the carry bit (bit 7)
  B = (B << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) | (B == 0 ? 0x80 : 0);
  F &= ~(0x40 | 0x20);  // Clear N, H flags
}

void SM83::rlc_c() {
  // Rotate C left through carry
  uint8_t carry = (C & 0x80) >> 7;  // Get the carry bit (bit 7)
  C = (C << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear N, H flags
}

void SM83::rlc_d() {
  // Rotate D left through carry
  uint8_t carry = (D & 0x80) >> 7;  // Get the carry bit (bit 7)
  D = (D << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rlc_e() {
  // Rotate E left through carry
  uint8_t carry = (E & 0x80) >> 7;  // Get the carry bit (bit 7)
  E = (E << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rlc_h() {
  // Rotate H left through carry
  uint8_t carry = (H & 0x80) >> 7;  // Get the carry bit (bit 7)
  H = (H << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rlc_l() {
  // Rotate L left through carry
  uint8_t carry = (L & 0x80) >> 7;  // Get the carry bit (bit 7)
  L = (L << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rlc_a() {
  // Rotate A left through carry
  uint8_t carry = (A & 0x80) >> 7;  // Get the carry bit (bit 7)
  A = (A << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rlc_hl_ptr(uint32_t &cycles) {
  // Rotate the value at memory address HL left through carry
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = (value & 0x80) >> 7;          // Get the carry bit (bit 7)
  value = (value << 1) | carry;  // Rotate left and set the new carry
  writeMemory(cycles, address,
              value);  // Write the rotated value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::rrc_b() {
  // Rotate B right through carry
  uint8_t carry = B & 0x01;     // Get the carry bit (bit 0)
  B = (B >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (B == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_c() {
  // Rotate C right through carry
  uint8_t carry = C & 0x01;     // Get the carry bit (bit 0)
  C = (C >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_d() {
  // Rotate D right through carry
  uint8_t carry = D & 0x01;     // Get the carry bit (bit 0)
  D = (D >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_e() {
  // Rotate E right through carry
  uint8_t carry = E & 0x01;     // Get the carry bit (bit 0)
  E = (E >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_h() {
  // Rotate H right through carry
  uint8_t carry = H & 0x01;     // Get the carry bit (bit 0)
  H = (H >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_l() {
  // Rotate L right through carry
  uint8_t carry = L & 0x01;     // Get the carry bit (bit 0)
  L = (L >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_a() {
  // Rotate A right through carry
  uint8_t carry = A & 0x01;     // Get the carry bit (bit 0)
  A = (A >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rrc_hl_ptr(uint32_t &cycles) {
  // Rotate the value at memory address HL right through carry
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = value & 0x01;                 // Get the carry bit (bit 0)
  value = (value >> 1) | (carry << 7);  // Rotate right and set the new carry
  writeMemory(cycles, address,
              value);  // Write the rotated value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::rl_b() {
  // Rotate B left through carry
  uint8_t carry = (B & 0x80) >> 7;  // Get the carry bit (bit 7)
  B = (B << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (B == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_c() {
  // Rotate C left through carry
  uint8_t carry = (C & 0x80) >> 7;  // Get the carry bit (bit 7)
  C = (C << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_d() {
  // Rotate D left through carry
  uint8_t carry = (D & 0x80) >> 7;  // Get the carry bit (bit 7)
  D = (D << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_e() {
  // Rotate E left through carry
  uint8_t carry = (E & 0x80) >> 7;  // Get the carry bit (bit 7)
  E = (E << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_h() {
  // Rotate H left through carry
  uint8_t carry = (H & 0x80) >> 7;  // Get the carry bit (bit 7)
  H = (H << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_l() {
  // Rotate L left through carry
  uint8_t carry = (L & 0x80) >> 7;  // Get the carry bit (bit 7)
  L = (L << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_a() {
  // Rotate A left through carry
  uint8_t carry = (A & 0x80) >> 7;  // Get the carry bit (bit 7)
  A = (A << 1) | carry;             // Rotate left and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rl_hl_ptr(uint32_t &cycles) {
  // Rotate the value at memory address HL left through carry
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = (value & 0x80) >> 7;          // Get the carry bit (bit 7)
  value = (value << 1) | carry;  // Rotate left and set the new carry
  writeMemory(cycles, address,
              value);  // Write the rotated value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::rr_b() {
  // Rotate B right through carry
  uint8_t carry = B & 0x01;     // Get the carry bit (bit 0)
  B = (B >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (B == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_c() {
  // Rotate C right through carry
  uint8_t carry = C & 0x01;     // Get the carry bit (bit 0)
  C = (C >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_d() {
  // Rotate D right through carry
  uint8_t carry = D & 0x01;     // Get the carry bit (bit 0)
  D = (D >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_e() {
  // Rotate E right through carry
  uint8_t carry = E & 0x01;     // Get the carry bit (bit 0)
  E = (E >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_h() {
  // Rotate H right through carry
  uint8_t carry = H & 0x01;     // Get the carry bit (bit 0)
  H = (H >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_l() {
  // Rotate L right through carry
  uint8_t carry = L & 0x01;     // Get the carry bit (bit 0)
  L = (L >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_a() {
  // Rotate A right through carry
  uint8_t carry = A & 0x01;     // Get the carry bit (bit 0)
  A = (A >> 1) | (carry << 7);  // Rotate right and set the new carry
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::rr_hl_ptr(uint32_t &cycles) {
  // Rotate the value at memory address HL right through carry
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = value & 0x01;                 // Get the carry bit (bit 0)
  value = (value >> 1) | (carry << 7);  // Rotate right and set the new carry
  writeMemory(cycles, address,
              value);  // Write the rotated value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::sla_b() {
  // Shift B left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (B & 0x80) >> 7;  // Get the carry bit (bit 7)
  B <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (B == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_c() {
  // Shift C left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (C & 0x80) >> 7;  // Get the carry bit (bit 7)
  C <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_d() {
  // Shift D left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (D & 0x80) >> 7;  // Get the carry bit (bit 7)
  D <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_e() {
  // Shift E left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (E & 0x80) >> 7;  // Get the carry bit (bit 7)
  E <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_h() {
  // Shift H left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (H & 0x80) >> 7;  // Get the carry bit (bit 7)
  H <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_l() {
  // Shift L left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (L & 0x80) >> 7;  // Get the carry bit (bit 7)
  L <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_a() {
  // Shift A left, filling bit 0 with 0 and storing the old bit 7 in the carry
  // flag
  uint8_t carry = (A & 0x80) >> 7;  // Get the carry bit (bit 7)
  A <<= 1;                          // Shift left
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sla_hl_ptr(uint32_t &cycles) {
  // Shift the value at memory address HL left, filling bit 0 with 0 and storing
  // the old bit 7 in the carry flag
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = (value & 0x80) >> 7;          // Get the carry bit (bit 7)
  value <<= 1;                                  // Shift left
  writeMemory(cycles, address,
              value);  // Write the shifted value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::sra_b() {
  // Shift B right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = B & 0x01;   // Get the carry bit (bit 0)
  B = (B >> 1) | (B & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (B == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_c() {
  // Shift C right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = C & 0x01;   // Get the carry bit (bit 0)
  C = (C >> 1) | (C & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_d() {
  // Shift D right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = D & 0x01;   // Get the carry bit (bit 0)
  D = (D >> 1) | (D & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_e() {
  // Shift E right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = E & 0x01;   // Get the carry bit (bit 0)
  E = (E >> 1) | (E & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_h() {
  // Shift H right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = H & 0x01;   // Get the carry bit (bit 0)
  H = (H >> 1) | (H & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_l() {
  // Shift L right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = L & 0x01;   // Get the carry bit (bit 0)
  L = (L >> 1) | (L & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_a() {
  // Shift A right, preserving the sign bit (bit 7) and storing the old bit 0 in
  // the carry flag
  uint8_t carry = A & 0x01;   // Get the carry bit (bit 0)
  A = (A >> 1) | (A & 0x80);  // Shift right and preserve the sign bit
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::sra_hl_ptr(uint32_t &cycles) {
  // Shift the value at memory address HL right, preserving the sign bit (bit 7)
  // and storing the old bit 0 in the carry flag
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = value & 0x01;                 // Get the carry bit (bit 0)
  value =
      (value >> 1) | (value & 0x80);  // Shift right and preserve the sign bit
  writeMemory(cycles, address,
              value);  // Write the shifted value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::swap_b() {
  // Swap the upper and lower nibbles of B
  B = (B << 4) | (B >> 4);     // Swap nibbles
  F = (B == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_c() {
  // Swap the upper and lower nibbles of C
  C = (C << 4) | (C >> 4);     // Swap nibbles
  F = (C == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_d() {
  // Swap the upper and lower nibbles of D
  D = (D << 4) | (D >> 4);     // Swap nibbles
  F = (D == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_e() {
  // Swap the upper and lower nibbles of E
  E = (E << 4) | (E >> 4);     // Swap nibbles
  F = (E == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_h() {
  // Swap the upper and lower nibbles of H
  H = (H << 4) | (H >> 4);     // Swap nibbles
  F = (H == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_l() {
  // Swap the upper and lower nibbles of L
  L = (L << 4) | (L >> 4);     // Swap nibbles
  F = (L == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_a() {
  // Swap the upper and lower nibbles of A
  A = (A << 4) | (A >> 4);     // Swap nibbles
  F = (A == 0 ? 0x80 : 0);     // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);  // Clear N, H, C flags
}

void SM83::swap_hl_ptr(uint32_t &cycles) {
  // Swap the upper and lower nibbles of the value at memory address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  value = (value << 4) | (value >> 4);          // Swap nibbles
  writeMemory(cycles, address,
              value);           // Write the swapped value back to memory
  F = (value == 0 ? 0x80 : 0);  // Set Z flag if result is zero
  F &= ~(0x40 | 0x20 | 0x10);   // Clear N, H, C flags
}

void SM83::srl_b() {
  // Shift B right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = B & 0x01;  // Get the carry bit (bit 0)
  B >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (B == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_c() {
  // Shift C right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = C & 0x01;  // Get the carry bit (bit 0)
  C >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (C == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_d() {
  // Shift D right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = D & 0x01;  // Get the carry bit (bit 0)
  D >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (D == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_e() {
  // Shift E right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = E & 0x01;  // Get the carry bit (bit 0)
  E >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (E == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_h() {
  // Shift H right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = H & 0x01;  // Get the carry bit (bit 0)
  H >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (H == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_l() {
  // Shift L right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = L & 0x01;  // Get the carry bit (bit 0)
  L >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (L == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_a() {
  // Shift A right, filling bit 7 with 0 and storing the old bit 0 in the carry
  // flag
  uint8_t carry = A & 0x01;  // Get the carry bit (bit 0)
  A >>= 1;                   // Shift right
  F = (carry ? (F & 0x10) : 0) |
      (A == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);      // Clear Z, N, H flags
}

void SM83::srl_hl_ptr(uint32_t &cycles) {
  // Shift the value at memory address HL right, filling bit 7 with 0 and
  // storing the old bit 0 in the carry flag
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  uint8_t carry = value & 0x01;                 // Get the carry bit (bit 0)
  value >>= 1;                                  // Shift right
  writeMemory(cycles, address,
              value);  // Write the shifted value back to memory
  F = (carry ? (F & 0x10) : 0) |
      (value == 0 ? 0x80 : 0);  // Set the carry flag if needed
  F &= ~(0x40 | 0x20);          // Clear Z, N, H flags
}

void SM83::bit_b_r(uint8_t bit) {
  // Test bit 'bit' of register B
  F = (B & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_c_r(uint8_t bit) {
  // Test bit 'bit' of register C
  F = (C & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_d_r(uint8_t bit) {
  // Test bit 'bit' of register D
  F = (D & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_e_r(uint8_t bit) {
  // Test bit 'bit' of register E
  F = (E & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_h_r(uint8_t bit) {
  // Test bit 'bit' of register H
  F = (H & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_l_r(uint8_t bit) {
  // Test bit 'bit' of register L
  F = (L & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_a_r(uint8_t bit) {
  // Test bit 'bit' of register A
  F = (A & (1 << bit)) ? (F & ~0x80) : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                                        // Set H flag
  F &= ~(0x40);                                     // Clear flags
}

void SM83::bit_hl_ptr(uint8_t bit, uint32_t &cycles) {
  // Test bit 'bit' of the value at memory address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  F = (value & (1 << bit)) ? (F & ~0x80)
                           : (F | 0x80);  // Set Z flag if bit is 0
  F |= 0x20;                              // Set H flag
  F &= ~(0x40);                           // Clear flags
}

void SM83::set_b_r(uint8_t bit) {
  // Set bit 'bit' of register B
  B |= (1 << bit);
}

void SM83::set_c_r(uint8_t bit) {
  // Set bit 'bit' of register C
  C |= (1 << bit);
}

void SM83::set_d_r(uint8_t bit) {
  // Set bit 'bit' of register D
  D |= (1 << bit);
}

void SM83::set_e_r(uint8_t bit) {
  // Set bit 'bit' of register E
  E |= (1 << bit);
}

void SM83::set_h_r(uint8_t bit) {
  // Set bit 'bit' of register H
  H |= (1 << bit);
}

void SM83::set_l_r(uint8_t bit) {
  // Set bit 'bit' of register L
  L |= (1 << bit);
}

void SM83::set_a_r(uint8_t bit) {
  // Set bit 'bit' of register A
  A |= (1 << bit);
}

void SM83::set_hl_ptr(uint8_t bit, uint32_t &cycles) {
  // Set bit 'bit' of the value at memory address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  value |= (1 << bit);                          // Set the specified bit
  writeMemory(cycles, address,
              value);  // Write the modified value back to memory
}

void SM83::res_b_r(uint8_t bit) {
  // Reset bit 'bit' of register B
  B &= ~(1 << bit);
}

void SM83::res_c_r(uint8_t bit) {
  // Reset bit 'bit' of register C
  C &= ~(1 << bit);
}

void SM83::res_d_r(uint8_t bit) {
  // Reset bit 'bit' of register D
  D &= ~(1 << bit);
}

void SM83::res_e_r(uint8_t bit) {
  // Reset bit 'bit' of register E
  E &= ~(1 << bit);
}

void SM83::res_h_r(uint8_t bit) {
  // Reset bit 'bit' of register H
  H &= ~(1 << bit);
}

void SM83::res_l_r(uint8_t bit) {
  // Reset bit 'bit' of register L
  L &= ~(1 << bit);
}

void SM83::res_a_r(uint8_t bit) {
  // Reset bit 'bit' of register A
  A &= ~(1 << bit);
}

void SM83::res_hl_ptr(uint8_t bit, uint32_t &cycles) {
  // Reset bit 'bit' of the value at memory address HL
  uint16_t address = (H << 8) | L;  // Combine H and L to get the address
  uint8_t value = readMemory(cycles, address);  // Read the value from memory
  value &= ~(1 << bit);                         // Reset the specified bit
  writeMemory(cycles, address,
              value);  // Write the modified value back to memory
}
