#pragma once

#include <array>
#include <cstdint>
#include <nlohmann/json.hpp>

#include "emulators/cpu/CPU.hh"
#include "emulators/cpu/Z80/Z80Bus.hh"

/**
 * @file Z80.hh
 * @brief Zilog Z80 CPU emulation (used by the Sega Master System / Game Gear).
 */

/**
 * @struct Z80State
 * @brief Serializable state of a Z80 CPU.
 * @details Register arrays use the encoding order B, C, D, E, H, L, F, A
 *          (index 6 holds the flags, which is never a register operand).
 */
struct Z80State {
  std::array<uint8_t, 8> reg{};  ///< B C D E H L F A
  std::array<uint8_t, 8> alt{};  ///< Shadow set: B' C' D' E' H' L' F' A'
  uint16_t IX = 0, IY = 0, SP = 0, PC = 0;
  uint16_t WZ = 0;  ///< Internal MEMPTR register (affects BIT n,(HL) flags).
  uint8_t I = 0, R = 0, IM = 0;
  bool IFF1 = false, IFF2 = false;
  bool halted = false;
  bool eiDelay = false;      ///< Interrupts blocked for one instruction (EI).
  bool nmiPending = false;   ///< NMI edge latched, not yet serviced.
  bool irqLine = false;      ///< Level of the /INT line.
  uint8_t irqData = 0xFF;    ///< Byte placed on the data bus on INT ack.
  uint8_t q = 0;             ///< Flags if the last instruction changed them.
  bool ldAIR = false;        ///< Last instruction was LD A,I / LD A,R.
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Z80State, reg, alt, IX, IY, SP, PC, WZ, I, R,
                                   IM, IFF1, IFF2, halted, eiDelay, nmiPending,
                                   irqLine, irqData, q, ldAIR)

/**
 * @class Z80
 * @brief Emulates the Zilog Z80, including its undocumented behaviour.
 *
 * Implemented: the full documented instruction set (main, CB, DD/FD, ED,
 * DDCB/FDCB pages), the undocumented instructions (IXH/IXL/IYH/IYL operands,
 * SLL, DDCB register copies, duplicate ED opcodes), the undocumented X/Y flag
 * bits, the MEMPTR (WZ) register, the Q-register behaviour of SCF/CCF, and
 * interrupt modes 0, 1 and 2 plus NMI.
 *
 * Flags (register F):
 *
 * | Bit | Flag | Meaning                 |
 * |-----|------|-------------------------|
 * | 7   | S    | Sign                    |
 * | 6   | Z    | Zero                    |
 * | 5   | Y    | Undocumented (bit 5)    |
 * | 4   | H    | Half-carry              |
 * | 3   | X    | Undocumented (bit 3)    |
 * | 2   | P/V  | Parity / overflow       |
 * | 1   | N    | Subtract                |
 * | 0   | C    | Carry                   |
 *
 * Cycle accounting: execute() returns T-states. Opcode fetches cost 4,
 * memory accesses 3, port accesses 4, plus the documented internal cycles.
 */
class Z80 : public CPU {
 public:
  /**
   * @brief Constructs the CPU.
   * @param bus Bus used for memory and I/O accesses (not owned).
   */
  explicit Z80(Z80Bus *bus);
  ~Z80() = default;

  /** @brief Resets the CPU to its power-up state (PC = 0, interrupts off). */
  void reset() override;

  /**
   * @brief Executes one instruction (including any DD/FD/ED/CB prefixes), or
   *        services one pending interrupt, or burns one HALT cycle.
   * @return Number of T-states consumed.
   */
  uint32_t execute() override;

  /**
   * @brief Reads a byte from memory (3 T-states).
   * @param cycles  Cycle accumulator, incremented by 3.
   * @param address 16-bit address.
   */
  uint8_t read(uint32_t &cycles, uint16_t address) override;

  /**
   * @brief Writes a byte to memory (3 T-states).
   * @param cycles  Cycle accumulator, incremented by 3.
   * @param address 16-bit address.
   * @param value   Byte to write.
   */
  void write(uint32_t &cycles, uint16_t address, uint8_t value) override;

  /** @brief Latches a non-maskable interrupt (edge on /NMI). */
  void requestNMI();

  /**
   * @brief Sets the level of the maskable /INT line.
   * @param level   True while the line is asserted (level-triggered).
   * @param dataBus Byte the device puts on the data bus during acknowledge
   *                (0xFF on the Sega Master System).
   */
  void setIRQ(bool level, uint8_t dataBus = 0xFF);

  /** @brief Returns the program counter (for debugging and tests). */
  uint16_t getPC() const { return pc_; }

  /** @brief Gets the current CPU state as a serializable structure. */
  Z80State getState() const;

  /** @brief Sets the CPU state from a serializable structure. */
  void setState(const Z80State &state);

 private:
  // Indices into the register arrays (encoding order of the opcodes).
  enum : int { rB = 0, rC, rD, rE, rH, rL, rF, rA };

  Z80Bus *bus_;  ///< System bus (not owned).

  std::array<uint8_t, 8> r_{};    ///< B C D E H L F A
  std::array<uint8_t, 8> alt_{};  ///< Shadow registers.
  uint16_t ix_ = 0, iy_ = 0, sp_ = 0, pc_ = 0, wz_ = 0;
  uint8_t i_ = 0, rr_ = 0, im_ = 0;
  bool iff1_ = false, iff2_ = false, halted_ = false, eiDelay_ = false;
  bool nmiPending_ = false, irqLine_ = false, ldAIR_ = false;
  uint8_t irqData_ = 0xFF;
  uint8_t q_ = 0, lastQ_ = 0;
  int idx_ = 0;  ///< Active index prefix for the current instruction: 0/1=IX/2=IY.

  /// @name Register helpers
  /// @{
  uint16_t hl() const { return static_cast<uint16_t>((r_[rH] << 8) | r_[rL]); }
  uint16_t pairBC() const { return static_cast<uint16_t>((r_[rB] << 8) | r_[rC]); }
  uint16_t pairDE() const { return static_cast<uint16_t>((r_[rD] << 8) | r_[rE]); }
  void setHL(uint16_t v);
  void setBC(uint16_t v);
  void setDE(uint16_t v);
  void setF(uint8_t f) {
    r_[rF] = f;
    q_ = f;
  }
  uint16_t &indexReg() { return idx_ == 1 ? ix_ : iy_; }
  uint8_t getR8(int i) const;  ///< 8-bit register honouring IXH/IXL/IYH/IYL.
  void setR8(int i, uint8_t v);
  uint16_t rpGet(int p) const;   ///< BC, DE, HL/IX/IY, SP.
  void rpSet(int p, uint16_t v);
  uint16_t rp2Get(int p) const;  ///< BC, DE, HL/IX/IY, AF.
  void rp2Set(int p, uint16_t v);
  bool cond(int cc) const;
  /// @}

  /// @name Memory, fetch and stack helpers (add T-states to @p c)
  /// @{
  uint8_t fetchOp(uint32_t &c);   ///< M1 cycle: 4 T, increments R.
  uint8_t fetch8(uint32_t &c);    ///< Operand byte: 3 T.
  uint16_t fetch16(uint32_t &c);  ///< Operand word: 6 T.
  void push(uint32_t &c, uint16_t v);
  uint16_t pop(uint32_t &c);
  uint8_t in(uint32_t &c, uint16_t port);
  void out(uint32_t &c, uint16_t port, uint8_t v);
  uint16_t indexAddr(uint32_t &c, uint32_t internal);
  uint16_t memOperand(uint32_t &c);  ///< (HL) or (IX/IY+d).
  /// @}

  /// @name ALU
  /// @{
  uint8_t add8(uint8_t a, uint8_t v, uint8_t carry);
  uint8_t sub8(uint8_t a, uint8_t v, uint8_t carry);
  void alu(int op, uint8_t v);
  uint8_t inc8(uint8_t v);
  uint8_t dec8(uint8_t v);
  uint16_t add16(uint16_t a, uint16_t b);
  void adc16(uint16_t b);
  void sbc16(uint16_t b);
  uint8_t rot(int y, uint8_t v);
  void bit(int n, uint8_t v, uint8_t xySource);
  void daa();
  /// @}

  /// @name Instruction groups
  /// @{
  void executeMain(uint8_t op, uint32_t &c);
  void executeCB(uint32_t &c);
  void executeIndexedCB(uint32_t &c);
  void executeED(uint32_t &c);
  void blockTransfer(int y, uint32_t &c);
  void blockCompare(int y, uint32_t &c);
  void blockIn(int y, uint32_t &c);
  void blockOut(int y, uint32_t &c);
  /// @}
};
