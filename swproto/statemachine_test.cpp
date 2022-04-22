#include <algorithm>
#include <bitset>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>
#include <sstream>

#include "font.hpp"
#include "statemachine.hpp"

std::string regs_of(const statemachine &mach) {
  using namespace std;
  stringstream reg_summary;
  reg_summary << "Regs: ";
  for (uint8_t reg : mach.regs()) {
    reg_summary << hex << setw(2) << setfill('0') << (int)reg << " ";
  }

  return reg_summary.str();
}

std::string
disp_str(std::span<const uint8_t, statemachine::DISPLAY_SIZE> display) {
  using namespace std;
  stringstream ret;
  ret << "display:\n";
  for (unsigned row = 0; row < statemachine::DISPLAY_HEIGHT; ++row) {
    cout << ' ';
    for (unsigned row_offset = 0; row_offset < statemachine::ROW_SIZE;
         ++row_offset) {
      cout << bitset<8>(display[row * statemachine::ROW_SIZE + row_offset]);
    }
    cout << '\n';
  }

  return ret.str();
}

std::string mem_of(const statemachine &mach) {
  using namespace std;
  stringstream mem_str;
  mem_str << "mem:";
  unsigned i = 0;
  for (uint8_t val : mach.memory()) {
    if ((i++ % 32) == 0) {
      mem_str << "\n  ";
    }
    mem_str << hex << setw(2) << setfill('0') << (int)val << " ";
  }
  return mem_str.str();
}

bool is_zero(uint8_t x) { return x == 0; }

inline void
ASSERT_STEP(statemachine &mach, uint16_t keystate, bool tick,
            statemachine::status expected_status = statemachine::NO_ERROR) {
  using namespace std;

  auto next_instruction = mach.curr_instruction();
  auto resultant_status = mach.step(keystate, tick);

  ASSERT_EQ(resultant_status, expected_status)
      << "while executing opcode 0x" << hex << setfill('0') << setw(4)
      << next_instruction;
}

TEST(StateMachineTest, Test00EE_2nnn) {
  std::initializer_list<uint16_t> instructions = {
      0x2004, // CALL 0x004
      0x1002, // JP 0x002 (hang in place)
      0x2008, // CALL 0x008
      0x00EE, // RET (when executed, should jump to JP instruction)
      0x00EE  // RET (should jump to above RET)
  };

  statemachine machine(instructions);
  ASSERT_STEP(machine, 0, 0);
  ASSERT_EQ(machine.pc(), 0x004);
  ASSERT_STEP(machine, 0, 0);
  ASSERT_EQ(machine.pc(), 0x008);
  ASSERT_STEP(machine, 0, 0);
  ASSERT_EQ(machine.pc(), 0x006);
  ASSERT_STEP(machine, 0, 0);
  ASSERT_EQ(machine.pc(), 0x002);
}

TEST(StateMachineTest, Test6xkk_7xkk) {
  statemachine machine({
      0x6789, // LD V7, 0x89
      0x7710, // ADD V7, 0x10
      0x77FF  // ADD V7, 0x77
  });
  EXPECT_EQ(machine.regs()[0x7], 0);
  ASSERT_STEP(machine, 0, false);
  EXPECT_EQ(machine.regs()[0x7], 0x89);
  ASSERT_STEP(machine, 0, false);
  EXPECT_EQ(machine.regs()[0x7], 0x99);
  ASSERT_STEP(machine, 0, false);
  EXPECT_EQ(machine.regs()[0x7], 0x98);
}

TEST(StateMachineTest, Test1nnn) {
  std::array<uint8_t, statemachine::MEMORY_SIZE> mem;
  mem.fill(0x66); // Fill with LD V6, 0x66
  // The below program should jump between JP instructions
  // and thus should never execute LD V6, 0x66

  // at 0x000: JP 0x020
  mem[0x000] = 0x10;
  mem[0x001] = 0x20;
  // at 0x020: JP 0x070
  mem[0x020] = 0x10;
  mem[0x021] = 0x70;
  // at 0x070: JP 0x100
  mem[0x070] = 0x11;
  mem[0x071] = 0x00;
  // at 0x100: JP 0x000
  mem[0x100] = 0x10;
  mem[0x101] = 0x00;

  statemachine machine(mem);

  // Make sure we never execute LD V6, 0x66
  for (int i = 0; i < 100; ++i) {
    ASSERT_STEP(machine, 0, false);
    ASSERT_EQ(machine.regs()[0x6], 0);
  }

  // Make sure memory was unchanged for no good reason.
  const auto &new_mem = machine.memory();
  ASSERT_TRUE(equal(mem.begin(), mem.end(), new_mem.begin(), new_mem.end()));
}

TEST(StateMachineTest, Test3xkk_4xkk) {
  std::initializer_list<uint16_t> instructions = {
      0x3000, // SE V0 == 0 (Should skip)
      0x6155, // LD V1, 0x55
      0x3001, // SE V0 == 1 (Should not skip)
      0x6255, // LD V2, 0x55
      0x4001, // SNE V0 == 1 (Should skip)
      0x6355, // LD V3, 0x55
      0x4000, // SNE V0 == 0 (Should not skip)
      0x6455, // LD V4, 0x55
  };

  statemachine machine(instructions);

  for (unsigned i = 0; i < 6 /* 2 of 8 instructions should be skipped */; ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.pc(), instructions.size() * 2)
      << "PC should sit after the final instruction";

  ASSERT_EQ(machine.regs()[0x0], 0) << "V0 should not have changed from 0";
  ASSERT_EQ(machine.regs()[0x1], 0) << "V1 should not have changed from 0";
  ASSERT_EQ(machine.regs()[0x2], 0x55) << "V2 should have changed from 0";
  ASSERT_EQ(machine.regs()[0x3], 0) << "V1 should not have changed from 0";
  ASSERT_EQ(machine.regs()[0x4], 0x55) << "V2 should have changed from 0";
}

TEST(StateMachineTest, Test5xy0_9xy0) {
  std::initializer_list<uint16_t> instructions = {
      0x6101, // LD V1, 0x01

      0x5110, // SE V1, V1 (should skip)
      0x6A55, // LD VA, 0x55
      0x5020, // SE V0, V2 (should skip)
      0x6A55, // LD VA, 0x55
      0x9010, // SNE V0, V1 (should skip)
      0x6A55, // LD VA, 0x55

      0x9110, // SNE V1, V1 (should not skip)
      0x6B55, // LD VB, 0x55
      0x9020, // SNE V0, V2 (should not skip)
      0x6C55, // LD VC, 0x55
      0x5010, // SE V0, V1 (should not skip)
      0x6D55, // LD VD, 0x55
  };

  statemachine machine(instructions);

  // Run until complete
  unsigned i;
  for (i = 0; (i < 20) && (machine.memory()[machine.pc()] != 0); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_NE(i, 20); // Make sure we didn't go somewhere we're not meant to be.

  ASSERT_EQ(machine.regs()[0xA], 0); // VA should never have been set.
  // VB, VC, and VD should have been set.
  ASSERT_EQ(machine.regs()[0xB], 0x55);
  ASSERT_EQ(machine.regs()[0xC], 0x55);
  ASSERT_EQ(machine.regs()[0xD], 0x55);
}

TEST(StateMachineTest, Test8xy0) {
  std::initializer_list<uint16_t> instructions = {
      0x6144, // LD V1, 0x44
      0x8310, // LD V3, V1
  };
  statemachine machine(instructions);

  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);

  ASSERT_EQ(machine.regs()[0x0], 0x00); // V0 should never have been set.
  ASSERT_EQ(machine.regs()[0x1], 0x44); // V1 should be 0x44
  ASSERT_EQ(machine.regs()[0x2], 0x00); // V0 should never have been set.
  ASSERT_EQ(machine.regs()[0x3], 0x44); // V3 should match V1
  // Remainder should not have been set.
  for (int i = 0x4; i <= 0xF; ++i) {
    ASSERT_EQ(machine.regs()[i], 0x00);
  }
}

TEST(StateMachineTest, Test8xy1_7xy2_8xy3) {
  std::initializer_list<uint16_t> instructions = {
      0x6105, // LD V1, 0x05
      0x62A0, // LD V2, 0xA0

      0x6A55, // LD VA, 0x55
      0x6B55, // LD VB, 0x55
      0x6C55, // LD VC, 0x55
      0x6D55, // LD VD, 0x55
      0x6E55, // LD VE, 0x55
      0x6F55, // LD VF, 0x55

      0x8A11, // OR VA, V1
      0x8B21, // OR VB, V2
      0x8C12, // AND VC, V1
      0x8D22, // AND VD, V2
      0x8E13, // XOR VE, V1
      0x8F23, // XOR VF, V2
  };

  statemachine machine(instructions);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  // Check if VA-VF have intended values.
  ASSERT_EQ(machine.regs()[0xA], 0x55);
  ASSERT_EQ(machine.regs()[0xB], 0xF5);
  ASSERT_EQ(machine.regs()[0xC], 0x05);
  ASSERT_EQ(machine.regs()[0xD], 0x00);
  ASSERT_EQ(machine.regs()[0xE], 0x50);
  ASSERT_EQ(machine.regs()[0xF], 0xF5);
}

TEST(StateMachineTest, Test8xy4) {
  statemachine machine({
      0x6082, // LD V0, 0x82
      0x6102, // LD V1, 0x02
      0x8104, // ADD V1, V0
      0x82F0, // LD V2, VF (save VF for later)
      0x8004, // ADD V0, V0 (should overflow)
  });

  for (int i = 0; i < 5; ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0x0], 0x04); // 0x82 + 0x82 = 0x104
  ASSERT_EQ(machine.regs()[0xF], 0x01); // Should have overflowed.
  ASSERT_EQ(machine.regs()[0x1], 0x84); // 0x82 + 0x02 = 0x84
  ASSERT_EQ(machine.regs()[0x2], 0x00); // Should not have overflowed.
}

TEST(StateMachineTest, Test8xy5) {
  statemachine machine({
      0x6092, // LD V0, 0x92
      0x6102, // LD V1, 0x02
      0x8015, // SUB V0, V1 (V0 = V0 - V1)
      0x8A00, // LD VA, V0 (save for later)
      0x8BF0, // LD VB, VF (save for later)
      0x6092, // LD V0, 0x92
      0x6102, // LD V1, 0x02
      0x8105, // SUB V1, V0 (V1 = V1 - V0)
  });

  for (int i = 0; i < 8; ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0xA], 0x90); // 0x92 - 0x02 = 0x90
  ASSERT_EQ(machine.regs()[0xB], 0x01); // 0x92 - 0x02 does not borrow.
  ASSERT_EQ(machine.regs()[0x1], 0x70); // 0x02 - 0x92 = 0x70
  ASSERT_EQ(machine.regs()[0xF], 0x00); // 0x02 - 0x92 DOES borrow.
}

TEST(StateMachineTest, Test8xy6) {
  { // Cases where VF should not be set.
    //
    // We choose immediates with 1's in most significant bit place to make we're
    // the rightshifts aren't sign-extending.
    std::initializer_list<uint16_t> instructions{
        0x6082, // LD V0 0x82
        0x6184, // LD V1 0x84
        0x8016, // SHR V0, V1
    };

    { // Case with shift quirks.
      statemachine machine(instructions, {.quirk_shift = true});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 0x41);
      ASSERT_EQ(machine.regs()[0xF], 0);
    }
    { // Case without shift quirks.
      statemachine machine(instructions, {.quirk_shift = false});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 0x42);
      ASSERT_EQ(machine.regs()[0xF], 0);
    }
  } // Cases where VF should be set.
  {
    std::initializer_list<uint16_t> instructions{
        0x6083, // LD V0 0x83
        0x6185, // LD V1 0x85
        0x8016, // SHR V0, V1
    };

    { // Case with shift quirks.
      statemachine machine(instructions, {.quirk_shift = true});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 0x41);
      ASSERT_EQ(machine.regs()[0xF], 1);
    }
    { // Case without shift quirks.
      statemachine machine(instructions, {.quirk_shift = false});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 0x42);
      ASSERT_EQ(machine.regs()[0xF], 1);
    }
  }
}

// Nearly identical to test for 8xy5
TEST(StateMachineTest, Test8xy7) {
  std::initializer_list<uint16_t> instructions = {
      0x6092, // LD V0, 0x92
      0x6102, // LD V1, 0x02
      0x8017, // SUBN V0, V1 (V0 = V1 - V0)
      0x8A00, // LD VA, V0 (save for later)
      0x8BF0, // LD VB, VF (save for later)
      0x6092, // LD V0, 0x92
      0x6102, // LD V1, 0x02
      0x8107, // SUBN V1, V0 (V1 = V0 - V1)
  };
  statemachine machine(instructions);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0xA], 0x70); // 0x02 - 0x92 = 0x70
  ASSERT_EQ(machine.regs()[0xB], 0x00); // 0x02 - 0x92 DOES borrow.
  ASSERT_EQ(machine.regs()[0x1], 0x90); // 0x92 - 0x02 = 0x90
  ASSERT_EQ(machine.regs()[0xF], 0x01); // 0x92 - 0x02 DOES NOT borrow.
}

TEST(StateMachineTest, Test8xyE) {
  { // Cases where VF should not be set.
    std::initializer_list<uint16_t> instructions{
        0x6005, // LD V0 0x05
        0x6107, // LD V1 0x07
        0x801E, // SHL V0, V1
    };

    { // Case with shift quirks.
      statemachine machine(instructions, {.quirk_shift = true});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 10);
      ASSERT_EQ(machine.regs()[0xF], 0);
    }
    { // Case without shift quirks.
      statemachine machine(instructions, {.quirk_shift = false});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 14);
      ASSERT_EQ(machine.regs()[0xF], 0);
    }
  } // Cases where VF should be set.
  {
    std::initializer_list<uint16_t> instructions{
        0x6085, // LD V0 0x85
        0x6187, // LD V1 0x87
        0x801E, // SHL V0, V1
    };

    { // Case with shift quirks.
      statemachine machine(instructions, {.quirk_shift = true});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 10);
      ASSERT_EQ(machine.regs()[0xF], 1);
    }
    { // Case without shift quirks.
      statemachine machine(instructions, {.quirk_shift = false});
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0x0], 14);
      ASSERT_EQ(machine.regs()[0xF], 1);
    }
  }
}

TEST(StateMachineTest, TestEx9E) {
  {
    std::initializer_list<uint16_t> instructions = {
        0x6209, // LD V2, 0x09
        0xE29E, // SKP V2
        0x60FF, // LD V0, 0xFF
        0x0000, // NOOP
    };

    { // Should skip b/c exact match.
      statemachine machine(instructions);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 1u << 9u, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0], 0x00);
    }

    { // Should not skip because does not match.
      statemachine machine(instructions);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 1u << 2u, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0], 0xFF);
    }
  }
  { // Test unreasonable requests.
    std::initializer_list<uint16_t> instructions = {
        0x6277, // LD V2, 0x77
        0xE29E, // SKP V2
        0x60FF, // LD V0, 0xFF
        0x0000, // NOOP
    };
    // Should not because 0x77 is not a real key and thus is never pressed.
    statemachine machine(instructions);
    ASSERT_STEP(machine, 0, false);
    ASSERT_STEP(machine, 0xFFFF /* Press all available keys */, false);
    ASSERT_STEP(machine, 0, false);
    ASSERT_EQ(machine.regs()[0], 0xFF);
  }
}

TEST(StateMachineTest, TestExA1) {
  { // Test cases for reasonable inputs.
    std::initializer_list<uint16_t> instructions = {
        0x6209, // LD V2, 0x09
        0xE2A1, // SKNP V2
        0x60FF, // LD V0, 0xFF
        0x0000, // NOOP
    };

    { // Should not skip b/c exact match.
      statemachine machine(instructions);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 1u << 9u, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0], 0xFF);
    }

    { // Should skip because does not match.
      statemachine machine(instructions);
      ASSERT_STEP(machine, 0, false);
      ASSERT_STEP(machine, 1u << 2u, false);
      ASSERT_STEP(machine, 0, false);
      ASSERT_EQ(machine.regs()[0], 0x00);
    }
  }

  { // Test cases for unreasonable inputs.
    // Should skip because 0x77 is not a real key and thus is never pressed.
    std::initializer_list<uint16_t> instructions = {
        0x6277, // LD V2, 0x77
        0xE2A1, // SKNP V2
        0x60FF, // LD V0, 0xFF
        0x0000, // NOOP
    };
    statemachine machine(instructions);
    ASSERT_STEP(machine, 0, false);
    ASSERT_STEP(machine, 0xFFFF, false);
    ASSERT_STEP(machine, 0, false);
    ASSERT_EQ(machine.regs()[0], 0x00);
  }
}

TEST(StateMachineTest, TestAnnn_Fx1E) {
  std::initializer_list<uint16_t> instructions = {
      0xAF10, // LD I, 0xF10
      0x60E0, // LD V0, 0xE0
      0xF01E, // ADD I, V0
      0xF01E, // ADD I, V0 (should rollover)
      0x0000, // Do nothing (allows m_reg_I to get masked)
  };
  statemachine machine(instructions);

  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), 0xF10);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), 0xFF0);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), 0x0D0);
}

TEST(StateMachineTest, TestFx0A) {
  std::initializer_list<uint16_t> instructions = {
      0xF00A, // LD V0, K
      0x0000, // NOOP
  };
  statemachine machine(instructions);

  // Spin a bit, making sure that the PC doesn't progress.
  for (unsigned i = 0; i < 100; ++i) {
    ASSERT_STEP(machine, 0, false, statemachine::WAITING_FOR_KEYPRESS);
    ASSERT_EQ(machine.pc(), 0) << "Machine should not progress.";
  }
  // Press key 7.
  ASSERT_STEP(machine, 1u << 7u, false);
  ASSERT_EQ(machine.pc(), 0x02) << "Machine should progress.";
  ASSERT_EQ(machine.regs()[0], 7);
}

TEST(StateMachineTest, TestFx29) {
  const uint16_t test_font_begin = 0x123;
  std::initializer_list<uint16_t> instructions = {
      0xF029, // LD F, V0
      0x6004, // LD V0, 0x4
      0xF029, // LD F, V0
  };
  statemachine machine(instructions, {.font_begin = test_font_begin});

  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), test_font_begin);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), test_font_begin + (4 * FONT_SPRITE_SIZE));
}

TEST(StateMachineTest, TestFx15_Fx18_Timers) {
  std::initializer_list<uint16_t> instructions = {
      0x6003, // LD V0, 0x03
      0x6102, // LD V1, 0x02
      0xF015, // LD DT, V0
      0xF118, // LD ST, V1

      0x0000, 0x0000, 0x0000, 0x0000 /* NO-OPs */
  };
  statemachine machine(instructions);

  // Make sure the values are loaded.
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_DT(), 0x03);
  ASSERT_EQ(machine.reg_ST(), 0x02);

  // Make sure clock ticks to, but not past, 0.
  ASSERT_STEP(machine, 0, true);
  ASSERT_EQ(machine.reg_DT(), 0x02);
  ASSERT_EQ(machine.reg_ST(), 0x01);
  ASSERT_STEP(machine, 0, true);
  ASSERT_EQ(machine.reg_DT(), 0x01);
  ASSERT_EQ(machine.reg_ST(), 0x00);
  ASSERT_STEP(machine, 0, true);
  ASSERT_EQ(machine.reg_DT(), 0x00);
  ASSERT_EQ(machine.reg_ST(), 0x00);
}

TEST(StateMachineTest, TestFx33) {
  std::initializer_list<uint16_t> instructions = {
      0xAF10,        // LD I, 0xF10
      0x6000 | 123u, // LD V0, 123
      0xF033,        // LD B, V0
  };
  statemachine machine(instructions);

  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);

  ASSERT_EQ(machine.memory()[machine.reg_I()], 1);
  ASSERT_EQ(machine.memory()[machine.reg_I() + 1], 2);
  ASSERT_EQ(machine.memory()[machine.reg_I() + 2], 3);
}

TEST(StateMachineTest, TestFx55) {
  std::initializer_list<uint16_t> instructions = {
      // Fill all the registers with random bytes
      0xC0FF, 0xC1FF, 0xC2FF, 0xC3FF, 0xC4FF, 0xC5FF, 0xC6FF, 0xC7FF,
      0xC8FF, 0xC9FF, 0xCAFF, 0xCBFF, 0xCCFF, 0xCDFF, 0xCEFF, 0xCFFF,
      0xAE00, // LD I, 0xE00
      0xF755, // LD [I], V7
      0xAE00, // LD I, 0xE00
      0xFF55, // LD [I], VF
  };

  for (int quirk_load_store = 0; quirk_load_store <= 1; ++quirk_load_store) {
    statemachine machine(instructions,
                         {.quirk_load_store = !!quirk_load_store});

    // Execute first 16 ops (that fill up V0..VF with randomness).
    for (unsigned i = 0; i < 16; ++i) {
      ASSERT_STEP(machine, 0, false);
    }

    std::array<uint8_t, 16> random_bytes;
    std::copy_n(machine.regs().begin(), 16, random_bytes.begin());

    auto mem = machine.memory();
    auto mem_random_begin = mem.begin() + 0xE00;

    ASSERT_STEP(machine, 0, false); // Executes LD I, 0xE00
    ASSERT_STEP(machine, 0, false); // Executes LD [I], V7

    if (quirk_load_store) {
      ASSERT_EQ(machine.reg_I(), 0xE00);
    } else {
      ASSERT_EQ(machine.reg_I(), 0xE08);
    }

    // Make sure that V0..V7 matches mem[0xE00]..mem[0xE07].
    ASSERT_TRUE(
        equal(random_bytes.begin(), random_bytes.begin() + 8, mem_random_begin))
        << mem_of(machine);
    // Make sure that 0xE08..0xFFF is empty.
    ASSERT_TRUE(std::all_of(mem_random_begin + 8, mem.end(), is_zero))
        << mem_of(machine);

    ASSERT_STEP(machine, 0, false); // Executes LD I, 0xE00
    ASSERT_STEP(machine, 0, false); // Executes LD [I], VF

    // Make sure that V0..VF matches mem[0xE00]..mem[0xE0F].
    ASSERT_TRUE(
        equal(random_bytes.begin(), random_bytes.end(), mem_random_begin))
        << mem_of(machine);
    // Make sure that mem[0xE0F]..mem[0xFFF] is empty.
    ASSERT_TRUE(std::all_of(mem_random_begin + 16, mem.end(), is_zero))
        << mem_of(machine);

    // Make sure register contents untouched.
    ASSERT_TRUE(equal(random_bytes.begin(), random_bytes.end(),
                      machine.regs().begin()));
  }
}

TEST(StateMachineTest, TestFx65) {
  std::initializer_list<uint16_t> instructions = {
      // Nonsense will begin 4 instructions from now.
      0xA008, // LD I, 0x008
      0xF765, // LD V7, [I]
      0xA008, // LD I, 0x008
      0xFF65, // LD VF, [I]
      // Fill 16 bytes with nonsense.
      0xDEAD, 0xBEEF, 0xF00D, 0xF1CE, 0xC0DE, 0xFACE, 0xFEED, 0xF00D};

  for (int quirk_load_store = 0; quirk_load_store <= 1; ++quirk_load_store) {
    statemachine machine(instructions,
                         {.quirk_load_store = !!quirk_load_store});

    ASSERT_STEP(machine, 0, false); // Executes LD I, 0x006
    ASSERT_STEP(machine, 0, false); // Executes LD V7, [0x006]

    if (quirk_load_store) {
      ASSERT_EQ(machine.reg_I(), 0x008);
    } else {
      ASSERT_EQ(machine.reg_I(), 0x008 + 7 + 1);
    }

    {
      // Make sure V0..V7 contains the first 8 bytes of nonsense
      // and that the remainder are empty.
      std::array<uint8_t, 16> expected_regs{0xDE, 0xAD, 0xBE, 0xEF,
                                            0xF0, 0x0D, 0xF1, 0xCE};
      ASSERT_TRUE(equal(expected_regs.begin(), expected_regs.end(),
                        machine.regs().begin()))
          << regs_of(machine);
    }

    ASSERT_STEP(machine, 0, false); // Executes LD I, 0x006
    ASSERT_STEP(machine, 0, false); // Executes LD VF, [0x006]
    {
      // Make sure all the regs are the same nonsense.
      std::array<uint8_t, 16> expected_regs{0xDE, 0xAD, 0xBE, 0xEF, 0xF0, 0x0D,
                                            0xF1, 0xCE, 0xC0, 0xDE, 0xFA, 0xCE,
                                            0xFE, 0xED, 0xF0, 0x0D};
      // Now make sure the regs are completely filled up with nonsense.
      ASSERT_TRUE(equal(expected_regs.begin(), expected_regs.end(),
                        machine.regs().begin()))
          << regs_of(machine);
    }
  }
}

TEST(StateMachineTest, TestCxkk) {
  // Fill all the registers with random bytes masked with DB.
  std::initializer_list<uint16_t> instructions = {
      0xC0DB, 0xC1DB, 0xC2DB, 0xC3DB, 0xC4DB, 0xC5DB, 0xC6DB, 0xC7DB,
      0xC8DB, 0xC9DB, 0xCADB, 0xCBDB, 0xCCDB, 0xCDDB, 0xCEDB, 0xCFDB,
  };
  statemachine machine(instructions);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
    // Check mask.
    ASSERT_EQ(machine.regs()[i] & ~0xDB, 0x00);
  }

  // Make sure that not all the registers are identical by making sure
  // they don't all match the first value.
  auto machine_regs = machine.regs();
  ASSERT_TRUE(std::any_of(machine_regs.begin(), machine_regs.end(),
                          [&](auto x) { return x != machine_regs.front(); }));
}

TEST(StateMachineTest, Test00E0_Dxyn) {
  std::initializer_list<uint16_t> instructions = {
      // Some sample data to mess with.
      0x0FF0, 0x8001,
      0xD011, // DRW V0, V0, 1
      0xD011, // DRW V0, V0, 1 (should undo previous instruction)
      0x6002, // LD V0, 0x02
      0xD011, // DRW V0, V1, 1
      0x00E0, // CLS
      0x6035, // LD V0, 0x3A (58)

  };
  statemachine machine(instructions, {.pc = 0x004});
  {
    auto display = machine.display();
    ASSERT_TRUE(std::all_of(display.begin(), display.end(), is_zero));
  }

  {
    ASSERT_STEP(machine, 0, 0); // Executes DRW V0, V1, 1
    auto display = machine.display();
    std::array<uint8_t, statemachine::DISPLAY_SIZE> expected_display{0x0F};
    ASSERT_TRUE(
        std::equal(display.begin(), display.end(), expected_display.begin()))
        << disp_str(display);
    ASSERT_FALSE(machine.regs()[0xF]);
  }
  {
    ASSERT_STEP(machine, 0, 0); // Executes DRW V0, V1, 1
    auto display = machine.display();
    ASSERT_TRUE(std::all_of(display.begin(), display.end(), is_zero));
    ASSERT_TRUE(machine.regs()[0xF]);
  }
  {
    ASSERT_STEP(machine, 0, 0); // Executes LD V0, 0x02
    ASSERT_STEP(machine, 0, 0); // Executes DRW V0, V1, 1
    auto display = machine.display();
    std::array<uint8_t, statemachine::DISPLAY_SIZE> expected_display{
        0b00000011, 0b11000000};
    ASSERT_TRUE(
        std::equal(display.begin(), display.end(), expected_display.begin()))
        << disp_str(display);
    ASSERT_FALSE(machine.regs()[0xf]);
  }
  {
    ASSERT_STEP(machine, 0, 0); // Executes CLS
    auto display = machine.display();
    ASSERT_TRUE(std::all_of(display.begin(), display.end(), is_zero));
  }
  {
    ASSERT_STEP(machine, 0, 0); // Executes LDs.
    auto display = machine.display();
    ASSERT_TRUE(std::all_of(display.begin(), display.end(), is_zero));
  }
}
