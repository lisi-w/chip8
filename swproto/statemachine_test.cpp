#include "statemachine.hpp"
#include <algorithm>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>
#include <sstream>

// Convenience functios for debugging.

std::string regs_of(const statemachine &mach) {
  using namespace std;
  stringstream reg_summary;
  reg_summary << "Regs: ";
  for (uint8_t reg : mach.regs()) {
    reg_summary << hex << setw(2) << setfill('0') << (int)reg << " ";
  }

  return reg_summary.str();
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

inline std::array<uint8_t, statemachine::MEMORY_SIZE>
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

/// Tests 6xkk and 7xkk.
TEST(StateMachineTest, Test6xkk_7xkk) {
  // Should put 0x89 in register V7,
  // then add 0x10 to it,
  // then add 0xFF to test rollover.
  statemachine machine({0x67, 0x89, 0x77, 0x10, 0x77, 0xFF}, 0, 0);
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

  statemachine machine(mem, 0, 0);

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

  statemachine machine(instructions_decode(instructions), 0, 0);

  for (unsigned i = 0; i < 6 /* 2 of 8 instructions should be skipped */; ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.pc(), instructions.size() * 2)
      << "PC should sit after the final instruction.";

  ASSERT_EQ(machine.regs()[0x0], 0) << "V0 should not have changed from 0\n";
  ASSERT_EQ(machine.regs()[0x1], 0) << "V1 should not have changed from 0\n";
  ASSERT_EQ(machine.regs()[0x2], 0x55) << "V2 should have changed from 0\n";
  ASSERT_EQ(machine.regs()[0x3], 0) << "V1 should not have changed from 0\n";
  ASSERT_EQ(machine.regs()[0x4], 0x55) << "V2 should have changed from 0\n";
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

  statemachine machine(instructions_decode(instructions), 0, 0);

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
  statemachine machine(instructions_decode(instructions), 0, 0);

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

  statemachine machine(instructions_decode(instructions), 0, 0);

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
  statemachine machine(instructions_decode({
                           0x6082, // LD V0, 0x82
                           0x6102, // LD V1, 0x02
                           0x8104, // ADD V1, V0
                           0x82F0, // LD V2, VF (save VF for later)
                           0x8004, // ADD V0, V0 (should overflow)
                       }),
                       0, 0);

  for (int i = 0; i < 5; ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0x0], 0x04); // 0x82 + 0x82 = 0x104
  ASSERT_EQ(machine.regs()[0xF], 0x01); // Should have overflowed.
  ASSERT_EQ(machine.regs()[0x1], 0x84); // 0x82 + 0x02 = 0x84
  ASSERT_EQ(machine.regs()[0x2], 0x00); // Should not have overflowed.
}

TEST(StateMachineTest, Test8xy5) {
  statemachine machine(instructions_decode({
                           0x6092, // LD V0, 0x92
                           0x6102, // LD V1, 0x02
                           0x8015, // SUB V0, V1 (V0 = V0 - V1)
                           0x8A00, // LD VA, V0 (save for later)
                           0x8BF0, // LD VB, VF (save for later)
                           0x6092, // LD V0, 0x92
                           0x6102, // LD V1, 0x02
                           0x8105, // SUB V1, V0 (V1 = V1 - V0)
                       }),
                       0, 0);

  for (int i = 0; i < 8; ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0xA], 0x90); // 0x92 - 0x02 = 0x90
  ASSERT_EQ(machine.regs()[0xB], 0x01); // 0x92 - 0x02 does not borrow.
  ASSERT_EQ(machine.regs()[0x1], 0x70); // 0x02 - 0x92 = 0x70
  ASSERT_EQ(machine.regs()[0xF], 0x00); // 0x02 - 0x92 DOES borrow.
}

TEST(StateMachineTest, Test8xy6) {
  // Choose 0x92 and 0x93 to make sure we're using logical right shifts.
  std::initializer_list<uint16_t> instructions = {
      0x6092, // LD V0, 0x92
      0x80A6, // SHR V0, VA (VA should be untouched)
      0x8AF0, // LD V1, VF (save for later)
      0x6293, // LD V2, 0x93
      0x82B6, // SHR V2, VB (VB should be untouched)
  };
  statemachine machine(instructions_decode(instructions), 0, 0);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0x0], 0x49); // (0x92 >> 2) = 0x49
  ASSERT_EQ(machine.regs()[0x1], 0x00); // Check VF
  ASSERT_EQ(machine.regs()[0xA], 0x00); // VA should be untouched.
  ASSERT_EQ(machine.regs()[0x2], 0x49); // (0x93 >> 2) = 0x49
  ASSERT_EQ(machine.regs()[0xF], 0x01); // Check VF
  ASSERT_EQ(machine.regs()[0xB], 0x00); // VB should be untouched.
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
  statemachine machine(instructions_decode(instructions), 0, 0);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0xA], 0x70); // 0x02 - 0x92 = 0x70
  ASSERT_EQ(machine.regs()[0xB], 0x00); // 0x02 - 0x92 DOES borrow.
  ASSERT_EQ(machine.regs()[0x1], 0x90); // 0x92 - 0x02 = 0x90
  ASSERT_EQ(machine.regs()[0xF], 0x01); // 0x92 - 0x02 DOES NOT borrow.
}

TEST(StateMachineTest, Test8xyE) {
  std::initializer_list<uint16_t> instructions = {
      0x6072, // LD V0, 0x72
      0x80AE, // SHL V0, VA (VA should be untouched)
      0x8AF0, // LD V1, VF (save for later)
      0x62F2, // LD V2, 0xF2
      0x82BE, // SHL V2, VB (VB should be untouched)
  };
  statemachine machine(instructions_decode(instructions), 0, 0);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  ASSERT_EQ(machine.regs()[0x0], 0xE4); // (0x72 << 2) = 0xE4
  ASSERT_EQ(machine.regs()[0x1], 0x00); // Check VF
  ASSERT_EQ(machine.regs()[0xA], 0x00); // VA should be untouched.
  ASSERT_EQ(machine.regs()[0x2], 0xE4); // (0xF2 << 2) = 0xE4
  ASSERT_EQ(machine.regs()[0xF], 0x01); // Check VF
  ASSERT_EQ(machine.regs()[0xB], 0x00); // VB should be untouched.
}

TEST(StateMachineTest, TestAnnn_Fx1E) {
  std::initializer_list<uint16_t> instructions = {
      0xAF10, // LD I, 0xF10
      0x60E0, // LD V0, 0xE0
      0xF01E, // ADD I, V0
      0xF01E, // ADD I, V0 (should rollover)
      0x0000, // Do nothing (allows m_reg_I to get masked)
  };
  statemachine machine(instructions_decode(instructions), 0, 0);

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
  statemachine machine(instructions_decode(instructions), 0, 0);

  // Spin a bit, making sure that the PC doesn't progress.
  for (unsigned i = 0; i < 100; ++i) {
    ASSERT_STEP(machine, 0, false, statemachine::WAITING_FOR_KEYPRESS);
    ASSERT_EQ(machine.pc(), 0) << "Machine should not progress.\n";
  }
  // Press key 7.
  ASSERT_STEP(machine, 1u << 7u, false);
  ASSERT_EQ(machine.pc(), 0x002) << "Machine should progress.\n";
  ASSERT_EQ(machine.regs()[0], 7);
}

TEST(StateMachineTest, TestFx29) {
  const uint16_t test_font_begin = 0x123;
  std::initializer_list<uint16_t> instructions = {
      0xF029, // LD F, V0
      0x6004, // LD V0, 0x4
      0xF029, // LD F, V0
  };
  statemachine machine(instructions_decode(instructions), 0, test_font_begin);

  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), test_font_begin);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_EQ(machine.reg_I(), test_font_begin + (4 * statemachine::FONT_ROWS));
}

TEST(StateMachineTest, TestFx15_Fx18_Timers) {
  std::initializer_list<uint16_t> instructions = {
      0x6003, // LD V0, 0x03
      0x6102, // LD V1, 0x02
      0xF015, // LD DT, V0
      0xF118, // LD ST, V1

      0x0000, 0x0000, 0x0000, 0x0000 /* NO-OPs */
  };
  statemachine machine(instructions_decode(instructions), 0, 0);

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
  statemachine machine(instructions_decode(instructions), 0, 0);

  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);
  ASSERT_STEP(machine, 0, false);

  ASSERT_EQ(machine.memory()[machine.reg_I()], 1);
  ASSERT_EQ(machine.memory()[machine.reg_I() + 1], 2);
  ASSERT_EQ(machine.memory()[machine.reg_I() + 2], 3);
}

TEST(StateMachineTest, TestFx55) {
  std::initializer_list<uint16_t> instructions = {
      // Fill all the registers with random bytes masked with DB.
      0xC0DB, 0xC1DB, 0xC2DB, 0xC3DB, 0xC4DB, 0xC5DB, 0xC6DB, 0xC7DB,
      0xC8DB, 0xC9DB, 0xCADB, 0xCBDB, 0xCCDB, 0xCDDB, 0xCEDB, 0xCFDB,
      0xAE00, // LD I, 0xE00
      0xF755, // LD [I], V7
      0xAF00, // LD I, 0xF00
      0xFF55, // LD [I], VF
  };
  statemachine machine(instructions_decode(instructions), 0, 0);

  for (unsigned i = 0; i < instructions.size(); ++i) {
    ASSERT_STEP(machine, 0, false);
  }

  auto is_zero = [](uint8_t x) { return x == 0; };
  auto machine_regs = machine.regs();
  auto machine_mem = machine.memory();

  auto mem_instructions_end = machine_mem.begin() + instructions.size() * 2;
  auto mem_V0_V7_begin = machine_mem.begin() + 0xE00;
  auto mem_V0_V7_end = machine_mem.begin() + (0xE07 + 1);
  auto mem_V0_VF_begin = machine_mem.begin() + 0xF00;
  auto mem_V0_VF_end = machine_mem.begin() + (0xF0F + 1);

  // Make sure that everything between the end of the instructions
  // and 0xDFF (inclusive) is 0.
  ASSERT_TRUE(std::all_of(mem_instructions_end, mem_V0_V7_begin, is_zero))
      << mem_of(machine);
  // Make sure that V0..V7 matches 0xE00..0xE07.
  ASSERT_TRUE(equal(mem_V0_V7_begin, mem_V0_V7_end, machine_regs.begin()))
      << mem_of(machine);
  // Make sure that 0xE08..0xDFF is empty.
  ASSERT_TRUE(std::all_of(mem_V0_V7_end, mem_V0_VF_begin, is_zero))
      << mem_of(machine);
  // Make sure that V0..VF matches 0xF00..0xF10.
  ASSERT_TRUE(equal(mem_V0_VF_begin, mem_V0_VF_end, machine_regs.begin()))
      << mem_of(machine);
  // Make sure that 0xF10..0xFFF is empty.
  ASSERT_TRUE(std::all_of(mem_V0_VF_end, machine_mem.end(), is_zero))
      << mem_of(machine);
}

TEST(StateMachineTest, TestFx65) {
  std::initializer_list<uint16_t> instructions = {
      // Nonsense will begin 4 instructions from now.
      0xA006, // LD I, 0x006
      0xF765, // LD V7, [I]
      0xFF65, // LD VF, [I]
      // Fill 16 bytes with nonsense.
      0xDEAD, 0xBEEF, 0x000F, 0xF1CE, 0xC0DE, 0xFACE, 0xFEED, 0xF00D};

  statemachine machine(instructions_decode(instructions), 0, 0);

  auto is_zero = [](uint8_t x) { return x == 0; };
  auto machine_regs = machine.regs();
  auto machine_mem = machine.memory();
  auto reg_V8_begin = machine_regs.begin() + 8;
  auto nonsense_begin = machine_mem.begin() + 0x006;

  ASSERT_STEP(machine, 0, false); // Executes LD V7, [0x006]
  ASSERT_STEP(machine, 0, false); // Executes LD V7, [0x006]

  // Make sure V0..V7 contains the first 8 bytes of nonsense.
  ASSERT_TRUE(equal(machine_regs.begin(), reg_V8_begin, nonsense_begin))
      << regs_of(machine);
  // Make sure V8..VF is still zero's.
  ASSERT_TRUE(std::all_of(reg_V8_begin, machine_regs.end(), is_zero))
      << regs_of(machine);

  ASSERT_STEP(machine, 0, false); // Executes LD VF, [0x006]
  // Now make sure the regs are completely filled up with nonsense.
  ASSERT_TRUE(equal(machine_regs.begin(), machine_regs.end(), nonsense_begin))
      << regs_of(machine);
}

TEST(StateMachineTest, TestCxkk) {
  // Fill all the registers with random bytes masked with DB.
  std::initializer_list<uint16_t> instructions = {
      0xC0DB, 0xC1DB, 0xC2DB, 0xC3DB, 0xC4DB, 0xC5DB, 0xC6DB, 0xC7DB,
      0xC8DB, 0xC9DB, 0xCADB, 0xCBDB, 0xCCDB, 0xCDDB, 0xCEDB, 0xCFDB,
  };
  statemachine machine(instructions_decode(instructions), 0, 0);

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
