#include "statemachine.hpp"
#include <cstddef>
#include <iomanip>
#include <iostream> // TODO: REMOVE
#include <mutex>
#include <random>
#include <vector>

static std::mt19937 random_generator;
static std::mutex random_mutex;

statemachine::statemachine(std::array<uint8_t, MEMORY_SIZE> mem, uint16_t pc,
                           uint16_t font_begin)
    : m_mem(mem), m_display{0}, m_regs{0}, m_stack{}, m_pc(pc),
      m_font_begin(font_begin), m_reg_I(0), m_reg_DT(0), m_reg_ST(0) {}

statemachine::status statemachine::step(uint16_t keystate, uint32_t ticks) {

  // m_pc & 0xFFF masks the program counter to 12 bit.
  // We bitshift it to the left since the i-th instruction starts at location
  // 2*i
  uint16_t pc_location = (m_pc & 0xFFF) << 1;

  // Opcodes are stored in most-significant-byte-first.
  uint16_t opcode = m_mem.at(pc_location | 1) |
                    (m_mem.at(static_cast<uint16_t>(pc_location)) << 8);

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

  // debugging code for safe keeping.
  std::cerr << "Running step with opcode  0x" << std::hex << std::setfill('0')
            << std::setw(4) << opcode << std::endl;
  std::cerr << "x: 0x" << std::hex << (int)x << ", kk: 0x" << std::hex
            << (int)kk << std::endl;

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
      m_pc += 2;
      return NO_ERROR;
    }
  } break;

  case 0x4: {
    if (m_regs.at(x) != kk) {
      m_pc += 2;
      return NO_ERROR;
    }
  } break;

  case 0x5: {
    if (m_regs.at(x) == m_regs.at(y)) {
      m_pc += 2;
      return NO_ERROR;
    }
  } break;

  case 0x6: {
    std::cerr << "caught 0x6\n";
    m_regs[x] = kk;
  } break;

  case 0x7: {
    std::cerr << "caught 0x7\n";
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
      m_regs[0xF] = sum >> 8;
    } break;

    case 0x5: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = (m_regs.at(x) > m_regs.at(y)) ? 1 : 0;
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
      m_regs[0xF] = (m_regs.at(y) > m_regs.at(x)) ? 1 : 0;
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
      return NOT_IMPLEMENTED;
    }
    break;
  } break;

  case 0x9: {
    if (m_regs.at(x) != m_regs.at(y)) {
      m_pc += 2;
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
        m_pc += 2;
        return NO_ERROR;
      }
    } break;
    case 0xA1: {
      if (!((keystate >> m_regs.at(x)) & 1)) {
        m_pc += 2;
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
      return NOT_IMPLEMENTED;
    }
    break;
  }

  ++m_pc;
  return NO_ERROR;
}

std::span<const uint16_t> statemachine::instruction_stack::const_view() const {
  return {c.begin(), c.size()};
}
