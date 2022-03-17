#ifndef SWIMP_STATEMACHINE_H
#define SWIMP_STATEMACHINE_H

#include <array>
#include <cstdint>
#include <stack>
#include <vector>

class statemachine {
public:
  const static int MEMORY_SIZE = 4096;
  const static int STACK_SIZE = 16;
  typedef std::array<uint16_t, MEMORY_SIZE / 2> instructions;
  typedef std::array<uint8_t, MEMORY_SIZE> memory;
  typedef std::array<uint8_t, 16> regs;
  typedef uint16_t instruction;
  typedef uint16_t keystate;

  enum status {
    NO_ERROR = 0,
    NOT_IMPLEMENTED = 1,
    POPPED_EMPTY_STACK = -2,
    PUSHED_FULL_STACK = -3,
    VFRACE = -4,
  };

  statemachine(instructions imem, instruction pc);

  /**
   * Executes one instruction
   */
  status execute(keystate state);

private:
  union alignas(8 /* Value should be at least 4 */) {
    memory m_mem;
    instructions m_imem;
  };
  regs m_regs;
  std::stack<instruction, std::vector<instruction>> m_stack;
  instruction m_pc;
  uint8_t m_reg_I;
};

#endif // SWIMP_STATEMACHINE_H
