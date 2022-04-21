#include "statemachine.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iomanip>
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
      std::fill(m_display.begin(), m_display.end(), 0);
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

    for (uint8_t mem_sprite_pos = m_reg_I, mem_sprite_end = m_reg_I + n,
                 row = vy, row_end = vy + n;
         row < row_end; ++row, ++mem_sprite_pos) {
      uint8_t sprite_row_contents = m_mem.at(mem_sprite_pos & 0xFFF);

      // In drawing each line of the sprite, we will cross a byte-boundary if
      // VX isn't divisible by 8. Thus, we have to flip the bits in each byte
      // across the boundary carefully.

      // To figure out which bits go where, we shift the sprite row
      // in a uint16_t which should span both possible bytes that our
      // shifted should now store in its upper byte the bits that will be
      // flipped in the first byte and the bits that will be flipped in the
      // second byte in the lower byte.
      // For example, for vx=3, when the sprite row contents are 0x10011001,
      // shifted should be 0b0001001100100000.
      uint16_t shifted = static_cast<uint16_t>(sprite_row_contents) << 8;
      shifted >>= vx & 0b111;

      // Now we fill the bytes spanned by this sprite.
      size_t first_idx = (row * DISPLAY_WIDTH) | (vx >> 3);
      size_t last_idx =
          (row * DISPLAY_WIDTH) | (((vx >> 3) + 1) % DISPLAY_WIDTH);

      auto first_iter = m_display.begin() + first_idx;
      auto last_iter = m_display.begin() + last_idx;

      if (*first_iter & (shifted >> 8)) {
        m_regs[0xF] = 1;
      }

      *first_iter ^= shifted >> 8;

      if (*last_iter & shifted) {
        m_regs[0xF] = 1;
      }
      *last_iter ^= shifted & 0xFF;
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
      if (!m_quirk_load_store) {
        m_reg_I += x + 1;
      }
    } break;

    case 0x65: {
      for (unsigned i = 0; i <= x; ++i) {
        m_regs[i] = m_mem.at((m_reg_I + i) & 0xFFF);
      }
      if (!m_quirk_load_store) {
        m_reg_I += x + 1;
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
