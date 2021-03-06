#ifndef SWIMP_STATEMACHINE_H
#define SWIMP_STATEMACHINE_H

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <stack>
#include <vector>

class statemachine {
public:
  const static unsigned MEMORY_SIZE = 4096;
  const static unsigned STACK_SIZE = 16;
  const static unsigned DISPLAY_WIDTH = 64;
  const static unsigned ROW_SIZE = DISPLAY_WIDTH / 8;
  const static unsigned DISPLAY_HEIGHT = 32;
  const static unsigned ROW_MASK = DISPLAY_HEIGHT - 1;
  const static unsigned ROW_OFFSET_MASK = ROW_SIZE - 1;
  const static unsigned DISPLAY_SIZE = ROW_SIZE * DISPLAY_HEIGHT;
  const static unsigned PROG_BEGIN = 0x200;

  // Non-negative statuses are those from which the state machine may recover.
  // Negative statuses are those with overflows.
  enum status {
    NO_ERROR = 0,
    NOT_IMPLEMENTED = 1,
    WAITING_FOR_KEYPRESS = 2,
    POPPED_EMPTY_STACK = -2,
    PUSHED_FULL_STACK = -3,
    MEMORY_OVERFLOW = -5,
    IMPOSSIBLE_KEYPRESS_REQUEST = -6,
    PC_UNALIGNED = -7,
    PC_OUT_OF_RANGE = -8,
    DEBUG_ERROR = -100, // Thrown for testing purposes.
  };

  struct init_conf {
    uint16_t pc;
    uint16_t font_begin;
    bool quirk_shift : 1;
    bool quirk_load_store : 1;
  };

  statemachine(std::array<uint8_t, MEMORY_SIZE> mem, init_conf conf = {});

  statemachine(std::initializer_list<uint16_t> instructions,
               init_conf conf = {});

  /**
   * Executes one instruction
   * @param ticks Number of 60Hz whole ticks that have passed since the last
   * invocation.
   */
  status step(uint16_t keystate, bool tick);

  /// Get current value of special register I.
  inline uint16_t reg_I() const { return m_reg_I; };

  /// Get current value of delay timer register.
  inline uint8_t reg_DT() const { return m_reg_DT; };

  /// Get current value of sound timer register.
  inline uint8_t reg_ST() const { return m_reg_ST; };

  /// Get current display
  inline const std::array<uint8_t, DISPLAY_SIZE> display() const {
    return m_display;
  };

  /// Get current memory
  inline std::span<const uint8_t, MEMORY_SIZE> memory() const { return m_mem; };

  /// Get current registers
  inline std::span<const uint8_t, 16> regs() const { return m_regs; };

  /// Get current program counter.
  inline uint16_t pc() const { return m_pc; };

  /// Get current stack
  inline std::span<const uint16_t> stack() const {
    return m_stack.const_view();
  };

  inline uint16_t curr_instruction() const {
    return (static_cast<uint16_t>(m_mem[m_pc]) << 8) |
           static_cast<uint16_t>(m_mem[m_pc + 1]);
  }

private:
  class instruction_stack : public std::stack<uint16_t, std::vector<uint16_t>> {
  public:
    std::span<const uint16_t> const_view() const;
  };

  std::array<uint8_t, MEMORY_SIZE> m_mem;
  std::array<uint8_t, DISPLAY_SIZE> m_display;
  std::array<uint8_t, 16> m_regs;
  instruction_stack m_stack;
  uint16_t m_pc;
  uint16_t m_font_begin;
  uint16_t m_reg_I;
  // Timer registers.
  uint8_t m_reg_DT; // delay timer.
  uint8_t m_reg_ST; // sound timer
  /* Quirk behavior detailed in
   * http://mir3z.github.io/chip8-emu/doc/chip8-cpu.js.html#sunlight-1-line-119
   */
  bool m_quirk_shift : 1;
  bool m_quirk_load_store : 1;
};

#endif // SWIMP_STATEMACHINE_H
