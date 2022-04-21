#include "statemachine.hpp"
#include <cassert>
#include <cstddef>
#include <iomanip>
#include <iostream> // TODO: REMOVE
#include <mutex>
#include <random>
#include <vector>

static std::mt19937 random_generator;
static std::mutex random_mutex;

inline static std::array<uint8_t, statemachine::MEMORY_SIZE>
instructions_decode(const std::initializer_list<uint16_t> instructions) {

  assert(instructions.size() <= (statemachine::MEMORY_SIZE / 2));

  std::array<uint8_t, statemachine::MEMORY_SIZE> mem{0};
  unsigned i = 0;
  for (uint16_t instruction : instructions) {
    // Necessary to do it this way b/c of endianness correctness.
    mem[i++] = instruction >> 8;
    mem[i++] = instruction & 0xFF;
  }

  return mem;
}

statemachine::statemachine(std::array<uint8_t, MEMORY_SIZE> mem,
                           statemachine::init_conf conf)
    : m_mem(mem), m_display{0}, m_regs{0}, m_stack{}, m_pc(conf.pc),
      m_font_begin(conf.font_begin & 0xFFF), m_reg_I(0), m_reg_DT(0),
      m_reg_ST(0), m_quirk_shift(conf.quirk_shift),
      m_quirk_load_store(conf.quirk_load_store) {}

statemachine::statemachine(std::initializer_list<uint16_t> instructions,
                           statemachine::init_conf conf)
    : m_mem(instructions_decode(instructions)), m_display{0}, m_regs{0},
      m_stack{}, m_pc(conf.pc), m_font_begin(conf.font_begin & 0xFFF),
      m_reg_I(0), m_reg_DT(0), m_reg_ST(0), m_quirk_shift(conf.quirk_shift),
      m_quirk_load_store(conf.quirk_load_store) {}

statemachine::status statemachine::step(uint16_t keystate, bool tick) {

  if (m_pc & 1) [[unlikely]] {
    return PC_UNALIGNED;
  }

  // Opcodes are stored in most-significant-byte-first.
  uint16_t opcode =
      m_mem.at(m_pc | 1) | (m_mem.at(static_cast<uint16_t>(m_pc)) << 8);

  uint16_t nnn = opcode & 0xFFF;
  uint16_t n = opcode & 0xF;
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t y = (opcode >> 4) & 0xF;
  uint8_t kk = opcode & 0xFF;

  m_reg_I &= 0xFFF;

  // Tick tick tick
  if (tick) {
    if (m_reg_DT > 0) {
      --m_reg_DT;
    }
    if (m_reg_ST > 0) {
      --m_reg_ST;
    }
  }

  // Grab first hexadigit.
  switch (opcode >> 12) {
  case 0x0: {
    if (opcode == 0x00E0) {
      m_display.reset();
    } else if (opcode == 0x00EE) {
      if (m_stack.empty()) [[unlikely]] {
        return POPPED_EMPTY_STACK;
      }
      m_pc = m_stack.top();
      m_stack.pop();
      return NO_ERROR;
    } else {
      // Ignore SYS
    }
  } break;

  case 0x1: {
    m_pc = nnn;
    return NO_ERROR;
  } break;

  case 0x2: {
    if (m_stack.size() == STACK_SIZE) [[unlikely]] {
      return PUSHED_FULL_STACK;
    } else {
      m_stack.push(m_pc);
      m_pc = nnn;
      return NO_ERROR;
    }
  }

  case 0x3: {
    if (m_regs.at(x) == kk) {
      m_pc += 4;
      return NO_ERROR;
    }
  } break;

  case 0x4: {
    if (m_regs.at(x) != kk) {
      m_pc += 4;
      return NO_ERROR;
    }
  } break;

  case 0x5: {
    if (m_regs.at(x) == m_regs.at(y)) {
      m_pc += 4;
      return NO_ERROR;
    }
  } break;

  case 0x6: {
    m_regs[x] = kk;
  } break;

  case 0x7: {
    m_regs[x] += kk;
  } break;

  case 0x8: {
    switch (opcode & 0xF) {
    case 0x0: {
      m_regs[x] = m_regs.at(y);
    } break;

    case 0x1: {
      m_regs[x] |= m_regs.at(y);
    } break;

    case 0x2: {
      m_regs[x] &= m_regs.at(y);
    } break;

    case 0x3: {
      m_regs[x] ^= m_regs.at(y);
    } break;

    case 0x4: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      uint16_t sum = static_cast<uint16_t>(m_regs.at(x)) +
                     static_cast<uint16_t>(m_regs.at(y));
      m_regs[x] = sum;
      m_regs[0xF] = (sum > 0xFF) ? 1 : 0;
    } break;

    case 0x5: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = (m_regs.at(x) > m_regs.at(y)) ? 1 : 0;
      m_regs[x] -= m_regs.at(y);
    } break;

    case 0x6: {
      if ((x == 0xF) || (y == 0xF)) [[unlikely]] {
        return VFRACE;
      }

      auto src_idx = m_quirk_shift ? x : y;

      m_regs[0xF] = m_regs.at(src_idx) & 1;
      m_regs[x] = m_regs.at(src_idx) >> 1;
    } break;

    case 0x7: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = (m_regs.at(y) > m_regs.at(x)) ? 1 : 0;
      m_regs[x] = m_regs.at(y) - m_regs.at(x);
    } break;

    case 0xE: {
      if ((x == 0xF) || (y == 0xF)) [[unlikely]] {
        return VFRACE;
      }

      auto src_idx = m_quirk_shift ? x : y;

      m_regs[0xF] = m_regs.at(src_idx) >> 7;
      m_regs[x] = m_regs.at(src_idx) << 1;
    } break;

    default:
      return NOT_IMPLEMENTED;
    }
    break;
  } break;

  case 0x9: {
    if (m_regs.at(x) != m_regs.at(y)) {
      m_pc += 4;
      return NO_ERROR;
    }
  } break;

  case 0xA: {
    m_reg_I = nnn;
  } break;

  case 0xB: {
    m_pc = nnn + m_regs.at(0);
    return NO_ERROR;
  } break;

  case 0xC: {
    std::scoped_lock lk(random_mutex);
    m_regs[x] = random_generator() & kk;
    break;
  }
  case 0xD: {
    if ((m_reg_I + n) > MEMORY_SIZE) {
      return MEMORY_OVERFLOW;
    }
    uint8_t vx = m_regs.at(x);
    uint8_t vy = m_regs.at(y);
    m_regs[0xF] = 0;

    uint16_t line = vy;
    uint16_t end_line = vy + n;
    uint16_t sprite_source = m_reg_I;
    uint16_t line_abs_begin_pos = vy * DISPLAY_WIDTH;

    while (line != end_line) {
      auto pos_x = vx;
      for (uint8_t bits_to_write = m_mem.at(sprite_source); bits_to_write;
           bits_to_write <<= 1, pos_x = (pos_x + 1) % DISPLAY_WIDTH) {

        auto pos = line_abs_begin_pos + pos_x;
        bool current_pixel = m_display[pos];
        bool bit_should_flip = bits_to_write & 0x80;
        if (current_pixel & bit_should_flip) {
          m_regs[0xF] = 1;
        }
        m_display[pos] = current_pixel ^ bit_should_flip;
      }
      ++line;
      ++sprite_source;
      line_abs_begin_pos =
          (line_abs_begin_pos + DISPLAY_WIDTH) % m_display.size();
    }
  } break;

  case 0xE:
    switch (kk) {
    case 0x9E: {
      if ((keystate >> m_regs.at(x)) & 1) {
        m_pc += 4;
        return NO_ERROR;
      }
    } break;
    case 0xA1: {
      if (!((keystate >> m_regs.at(x)) & 1)) {
        m_pc += 4;
        return NO_ERROR;
      }
    } break;
    default: {
      return NOT_IMPLEMENTED;
    }
    }
    break;

  case 0xF:
    switch (kk) {
    case 0x07: {
      m_regs[x] = m_reg_DT;
    } break;

    case 0x0A: {
      if (keystate) {
        for (unsigned i = 0, mask = 1; i < 16; ++i, mask <<= 1) {
          if (mask & keystate) {
            m_regs[x] = i;
            break;
          }
        }
      } else {
        // Returning early means that PC isn't incremented
        return WAITING_FOR_KEYPRESS;
      }
    } break;

    case 0x15: {
      m_reg_DT = m_regs.at(x);
    } break;

    case 0x18: {
      m_reg_ST = m_regs.at(x);
    } break;

    case 0x1E: {
      m_reg_I += m_regs.at(x);
    } break;

    case 0x29: {
      m_reg_I = m_font_begin + (m_regs.at(x) * FONT_ROWS);
    } break;

    case 0x33: {
      auto vx = m_regs.at(x);
      m_mem[(m_reg_I + 2) & 0xFFF] = vx % 10;
      vx /= 10;
      m_mem[(m_reg_I + 1) & 0xFFF] = vx % 10;
      vx /= 10;
      m_mem[m_reg_I & 0xFFF] = vx % 10;
    } break;

    case 0x55: {
      for (unsigned i = 0; i <= x; ++i) {
        m_mem[(m_reg_I + i) & 0xFFF] = m_regs.at(i);
      }
    } break;

    case 0x65: {
      for (unsigned i = 0; i <= x; ++i) {
        m_regs[i] = m_mem.at((m_reg_I + i) & 0xFFF);
      }
    } break;
    default:
      return NOT_IMPLEMENTED;
    }
    break;
  }

  m_pc += 2;
  return NO_ERROR;
}

std::span<const uint16_t> statemachine::instruction_stack::const_view() const {
  return {c.begin(), c.size()};
}
