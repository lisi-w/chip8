#ifndef SWIMP_STATEMACHINE_H
#define SWIMP_STATEMACHINE_H

#include <array>
#include <bitset>
#include <cstdint>
#include <span>
#include <stack>
#include <vector>

class statemachine {
public:
  const static int MEMORY_SIZE = 4096;
  const static int STACK_SIZE = 16;
  const static int DISPLAY_WIDTH = 64;
  const static int DISPLAY_HEIGHT = 32;

  enum status {
    NO_ERROR = 0,
    NOT_IMPLEMENTED = 1,
    WAITING_FOR_KEYPRESS = 2,
    POPPED_EMPTY_STACK = -2,
    PUSHED_FULL_STACK = -3,
    VFRACE = -4,
    MEMORY_OVERFLOW = -5,
    DISPLAY_OVERFLOW = -6,
    IMPOSSIBLE_KEYPRESS_REQUEST = -6,
  };

  statemachine(std::array<uint8_t, MEMORY_SIZE> mem, uint16_t pc,
               uint16_t font_begin);

  /**
   * Executes one instruction
   * @param ticks Number of 60Hz whole ticks that have passed since the last
   * invocation.
   */
  status step(uint16_t keystate, uint32_t ticks);

  /// Get current value of delay timer register.
  inline uint8_t reg_DT() const { return m_reg_DT; };

  /// Get current value of sound timer register.
  inline uint8_t reg_ST() const { return m_reg_ST; };

  /// Get current display
  inline const std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> &display() const {
    return m_display;
  };

  /// Get current memory
  inline std::span<const uint8_t, MEMORY_SIZE> memory() const { return m_mem; };

  /// Get current program counter.
  inline uint16_t pc() const { return m_pc; };

  /// Get current stack
  inline std::span<const uint16_t> stack() const {
    return m_stack.const_view();
  };

private:
  class instruction_stack : public std::stack<uint16_t, std::vector<uint16_t>> {
  public:
    std::span<const uint16_t> const_view() const;
  };

  std::array<uint8_t, MEMORY_SIZE> m_mem;
  std::bitset<DISPLAY_WIDTH * DISPLAY_HEIGHT> m_display;
  std::array<uint8_t, 16> m_regs;
  instruction_stack m_stack;
  uint16_t m_pc;
  uint16_t m_font_begin;
  uint16_t m_reg_I;
  // Timer registers.
  uint8_t m_reg_DT; // delay timer.
  uint8_t m_reg_ST; // sound timer
};

#endif // SWIMP_STATEMACHINE_H
