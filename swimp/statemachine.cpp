#include "statemachine.hpp"
#include <cstddef>
#include <mutex>
#include <random>

static std::mt19937 random_generator;
static std::mutex random_mutex;

statemachine::statemachine(std::array<uint8_t, MEMORY_SIZE> mem, uint16_t pc,
                           uint16_t font_begin)
    : m_mem(mem), m_display{0}, m_pc(pc), m_font_begin(font_begin), m_reg_I(0),
      m_reg_DT(0), m_reg_ST(0) {}

statemachine::status statemachine::step(uint16_t keystate, uint32_t ticks) {
  statemachine::status ret = NO_ERROR;

  // m_pc & 0xFFF masks the program counter, which is 12 bit.
  auto pc_location = (m_pc & 0xFFF) << 1;
  // Opcodes are stored in most-significant-byte-first.
  uint16_t opcode =
      (m_mem.at(pc_location | 1)) | (static_cast<uint16_t>(pc_location) << 8);

  uint16_t nnn = opcode & 0xFFF;
  uint16_t n = opcode & 0xF;
  uint8_t x = (opcode >> 8) & 0xF;
  uint8_t y = (opcode >> 4) & 0xF;
  uint8_t kk = opcode & 0xFF;

  // Tick tick tick
  if (m_reg_DT < ticks) {
    m_reg_DT = 0;
  } else {
    m_reg_DT -= ticks;
  }
  if (m_reg_ST < ticks) {
    m_reg_ST = 0;
  } else {
    m_reg_ST -= ticks;
  }

  switch (opcode & 0xF000) {
  case 0x0: {
    if (opcode == 0x00E0) {
      ret = NOT_IMPLEMENTED;
    } else if (opcode == 0x00EE) {
      if (m_stack.empty()) [[unlikely]] {
        return POPPED_EMPTY_STACK;
      }
      m_pc = m_stack.top();
      m_stack.pop();
      return ret;
    } else {
      // Ignore SYS
    }
  } break;

  case 0x1: {
    m_pc = nnn;
    return ret;
  } break;

  case 0x2: {
    if (m_stack.size() == STACK_SIZE) [[unlikely]] {
      return PUSHED_FULL_STACK;
    }
    m_stack.push(m_pc);
    m_pc = nnn;
  } break;

  case 0x3: {
    if (m_regs.at(x) == (opcode & 0xFF)) {
      ++m_pc;
    }
  } break;
  case 0x4: {
    if (m_regs.at(x) != (opcode & 0xFF)) {
      ++m_pc;
    }
  } break;

  case 0x5: {
    if (m_regs.at(x) == m_regs.at(y)) {
      ++m_pc;
    }
  } break;

  case 0x6: {
    m_regs[x] = opcode & 0xFF;
  } break;

  case 0x7: {
    m_regs[x] += opcode & 0xFF;
  } break;

  case 0x8: {
    switch (opcode & 0xF) {
    case 0x0:
      m_regs[x] = m_regs.at(y);
      break;

    case 0x1:
      m_regs[x] |= m_regs.at(y);
      break;

    case 0x2:
      m_regs[x] &= m_regs.at(y);
      break;

    case 0x3:
      m_regs[x] ^= m_regs.at(y);
      break;

    case 0x4: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      uint16_t sum = static_cast<uint16_t>(m_regs.at(x)) +
                     static_cast<uint16_t>(m_regs.at(y));
      m_regs[x] = sum;
      m_regs[0xF] = sum >> 8;
    } break;

    case 0x5: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      bool not_borrow = m_regs.at(x) > m_regs.at(y);
      m_regs[0xF] = not_borrow ? 1 : 0;
      m_regs[x] -= m_regs.at(y);
    } break;

    case 0x6: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = m_regs.at(x) & 1;
      m_regs[x] >>= 1;
    } break;

    case 0x7: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      bool not_borrow = m_regs.at(y) > m_regs.at(x);
      m_regs[0xF] = not_borrow ? 1 : 0;
      m_regs[x] = m_regs.at(y) - m_regs.at(x);
    } break;

    case 0xE: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = m_regs.at(x) >> 7;
      m_regs[x] <<= 1;
    } break;

    default:
      ret = NOT_IMPLEMENTED;
    }
    break;
  } break;

  case 0x9: {
    if (m_regs.at(x) != m_regs.at(y)) {
      ++m_pc;
    }
  } break;

  case 0xA: {
    m_reg_I = nnn;
  } break;

  case 0xB: {
    m_pc = nnn + m_regs.at(0);
    return ret;
  } break;

  case 0xC: {
    std::scoped_lock lk(random_mutex);
    m_regs[x] = random_generator() & 0xFF;
    break;
  }
  case 0xD: {
    if ((m_reg_I + n) > MEMORY_SIZE) {
      return MEMORY_OVERFLOW;
    }
    uint8_t vx = m_regs.at(x);
    uint8_t vy = m_regs.at(y);
    m_regs[0xF] = 0;

    for (uint16_t line = vy, end_line = (line + n), sprite_source = m_reg_I,
                  line_begin_pos = vx + vy * DISPLAY_WIDTH;
         line < end_line;
         ++line, ++sprite_source, line_begin_pos += DISPLAY_WIDTH) {
      auto pos = line_begin_pos;
      for (uint8_t bits_to_write = m_mem.at(sprite_source); bits_to_write;
           bits_to_write <<= 1, ++pos) {
        if (pos >= m_display.size()) {
          return DISPLAY_OVERFLOW;
        }
        bool current_pixel = m_display.at(pos);
        bool bit_should_flip = bits_to_write & 0x80;
        if (current_pixel & bit_should_flip) {
          m_regs[0xF] = 1;
        }
        m_display.at(pos) = current_pixel ^ bit_should_flip;
      }
    }
  } break;

  case 0xE:
    switch (kk) {
    case 0x9E: {
      if ((keystate >> m_regs.at(x)) & 1) {
        ++m_pc;
      }
    } break;
    case 0xA1: {
      if (!((keystate >> m_regs.at(x)) & 1)) {
        ++m_pc;
      }
    } break;
    default: {
      ret = NOT_IMPLEMENTED;
    }
    }
    break;

  case 0xF:
    switch (kk) {
    case 0x07: {
      m_regs[x] = m_reg_DT;
    } break;

    case 0x0A: {
      auto vx = m_regs.at(x);
      if (vx > 15) {
        return IMPOSSIBLE_KEYPRESS_REQUEST;
      } else if (!((keystate >> vx) & 1)) {
        // Returning early means that PC isn't incremented
        // so the next step() call comes back here.
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
      m_reg_I = m_font_begin * (m_regs.at(x) * 5);
    } break;

    case 0x33: {
      if (m_reg_I + 2 > MEMORY_SIZE) {
        return MEMORY_OVERFLOW;
      }
      auto vx = m_regs.at(x);
      m_mem[m_reg_I + 2] = vx % 10;
      vx /= 10;
      m_mem[m_reg_I + 1] = vx % 10;
      vx /= 10;
      m_mem[m_reg_I] = vx % 10;
    } break;

    case 0x55: {
      auto mem_begin = m_mem.begin() + (m_reg_I & 0xFFF);
      auto n_to_copy = x + 1;
      // Make sure we don't copy past the end.
      if (n_to_copy > (m_mem.cend() - mem_begin)) [[unlikely]] {
        return MEMORY_OVERFLOW;
      }
      std::copy(m_regs.cbegin(), m_regs.cbegin() + n_to_copy, mem_begin);
    } break;

    case 0x65: {
      auto mem_begin = m_mem.cbegin() + (m_reg_I & 0xFFF);
      auto n_to_copy = x + 1;
      // Make sure we don't copy past the end.
      if (n_to_copy > (m_mem.cend() - mem_begin)) [[unlikely]] {
        return MEMORY_OVERFLOW;
      }
      std::copy(mem_begin, mem_begin + n_to_copy, m_regs.begin());
    } break;
    default:
      ret = NOT_IMPLEMENTED;
    }
    break;
  }
  ++m_pc;
  return NOT_IMPLEMENTED;
}
