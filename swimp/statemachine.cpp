#include "statemachine.hpp"
#include <mutex>
#include <random>

static std::mt19937 random_generator;
static std::mutex random_mutex;

statemachine::statemachine(instructions imem, instruction pc)
    : m_imem(imem), m_pc(pc) {}

statemachine::status statemachine::execute(statemachine::keystate /*state*/) {
  statemachine::status ret = NO_ERROR;
  instruction in = m_imem.at(m_pc & 0xFFF /* Mask off program counter */);
  uint16_t nnn = in & 0xFFF;
  uint8_t x = (in >> 8) & 0xF;
  uint8_t y = (in >> 4) & 0xF;
  uint8_t kk = 0xFF;

  switch (in & 0xF000) {
  case 0x0:
    if (in == 0x00E0) {
      ret = NOT_IMPLEMENTED;
    } else if (in == 0x00EE) {
      if (m_stack.empty()) [[unlikely]] {
        return POPPED_EMPTY_STACK;
      }
      m_pc = m_stack.top();
      m_stack.pop();
      return ret;
    } else {
      // Ignore SYS
    }
    break;
  case 0x1:
    m_pc = nnn;
    return ret;
    break;
  case 0x2:
    if (m_stack.size() == STACK_SIZE) [[unlikely]] {
      return PUSHED_FULL_STACK;
    }
    m_stack.push(m_pc);
    m_pc = nnn;
    break;
  case 0x3:
    if (m_regs.at(x) == (in & 0xFF)) {
      ++m_pc;
    }
    break;
  case 0x4:
    if (m_regs.at(x) != (in & 0xFF)) {
      ++m_pc;
    }
    break;
  case 0x5:
    if (m_regs.at(x) == m_regs.at(y)) {
      ++m_pc;
    }
    break;
  case 0x6:
    m_regs[x] = in & 0xFF;
    break;
  case 0x7:
    m_regs[x] += in & 0xFF;
    break;
  case 0x8:
    switch (in & 0xF) {
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
      break;
    }
    case 0x5: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      bool not_borrow = m_regs.at(x) > m_regs.at(y);
      m_regs[0xF] = not_borrow ? 1 : 0;
      m_regs[x] -= m_regs.at(y);
      break;
    }
    case 0x6: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = m_regs.at(x) & 1;
      m_regs[x] >>= 1;
      break;
    }
    case 0x7: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      bool not_borrow = m_regs.at(y) > m_regs.at(x);
      m_regs[0xF] = not_borrow ? 1 : 0;
      m_regs[x] = m_regs.at(y) - m_regs.at(x);
      break;
    }
    case 0xE: {
      if (x == 0xF) [[unlikely]] {
        return VFRACE;
      }
      m_regs[0xF] = m_regs.at(x) >> 7;
      m_regs[x] <<= 1;
      break;
    }
    default:
      ret = NOT_IMPLEMENTED;
    }
    break;
  case 0x9:
    if (m_regs.at(x) != m_regs.at(y)) {
      ++m_pc;
    }
    break;
  case 0xA:
    m_reg_I = nnn;
    break;
  case 0xB:
    m_pc = nnn + m_regs.at(0);
    return ret;
    break;
  case 0xC: {
    std::scoped_lock lk(random_mutex);
    m_regs[x] = random_generator() & 0xFF;
    break;
  }
  case 0xD:
    ret = NOT_IMPLEMENTED;
    break;
  case 0xE:
    ret = NOT_IMPLEMENTED;
    break;
  case 0xF:
    ret = NOT_IMPLEMENTED;
    break;
  }
  ++m_pc;
  return statemachine::NOT_IMPLEMENTED;
}
