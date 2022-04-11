#include "statemachine.hpp"
#include <algorithm>
#include <gtest/gtest.h>
#include <initializer_list>

// Convenience functios for debugging.

void print_regs(const statemachine &mach) {
  using namespace std;
  cerr << "Regs: ";
  for (uint8_t reg : mach.regs()) {
    cerr << hex << setw(2) << setfill('0') << (int)reg << " ";
  }
  cerr << endl;
}

inline void
ASSERT_STEP(statemachine &mach, uint16_t keystate, uint32_t ticks,
            statemachine::status expected_status = statemachine::NO_ERROR) {
  using namespace std;

  auto next_instruction = mach.curr_instruction();
  auto resultant_status = mach.step(keystate, ticks);

  ASSERT_EQ(resultant_status, expected_status)
      << "Executing opcode 0x" << hex << setfill('0') << setw(4) << "\t Got "
      << resultant_status << " but expected " << expected_status << ".\n";
}

inline std::array<uint8_t, statemachine::MEMORY_SIZE>
instruction_decode(const std::initializer_list<uint16_t> instructions) {

  assert(instructions.size() <= (statemachine::MEMORY_SIZE / 2));

  std::array<uint8_t, statemachine::MEMORY_SIZE> mem;
  unsigned i = 0;
  for (uint16_t instruction : instructions) {
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
  EXPECT_EQ(machine.step(0, 0), statemachine::NO_ERROR);
  EXPECT_EQ(machine.regs()[0x7], 0x89);
  EXPECT_EQ(machine.step(0, 0), statemachine::NO_ERROR);
  EXPECT_EQ(machine.regs()[0x7], 0x99);
  EXPECT_EQ(machine.step(0, 0), statemachine::NO_ERROR);
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
    ASSERT_EQ(machine.step(0, 1), statemachine::NO_ERROR);
    ASSERT_EQ(machine.regs()[0x6], 0);
  }

  // Make sure memory was unchanged for no good reason.
  const auto &new_mem = machine.memory();
  ASSERT_TRUE(equal(mem.begin(), mem.end(), new_mem.begin(), new_mem.end()));
}

TEST(StateMachineTest, Test3xkk_4xkk) {
  statemachine machine(
      {
          0x30, 0x00, // SE V0 == 0 (Should skip)
          0x61, 0x55, // LD V1, 0x55
          0x30, 0x01, // SE V0 == 1 (Should not skip)
          0x62, 0x55, // LD V2, 0x55
          0x40, 0x01, // SNE V0 == 1 (Should skip)
          0x63, 0x55, // LD V3, 0x55
          0x40, 0x00, // SNE V0 == 0 (Should not skip)
          0x64, 0x55, // LD V4, 0x55
      },
      0, 0);

  // 6 instruction should execute.
  for (int i = 0; i < 6; ++i) {
    ASSERT_EQ(machine.step(0, 1), statemachine::NO_ERROR);
  }

  // PC should sit right after the last LD.
  ASSERT_EQ(machine.pc(), 0x010);

  // V0 should not have changed from 0
  ASSERT_EQ(machine.regs()[0x0], 0);
  // V1 should not have changed from 0
  ASSERT_EQ(machine.regs()[0x1], 0);
  // V2 should indeed have changed from 0
  ASSERT_EQ(machine.regs()[0x2], 0x55);
  // V1 should not have changed from 0
  ASSERT_EQ(machine.regs()[0x3], 0);
  // V2 should indeed have changed from 0
  ASSERT_EQ(machine.regs()[0x4], 0x55);
}

TEST(StateMachineTest, Test5xy0_9xy0) {
  statemachine machine(
      {
          0x61, 0x01, // LD V1, 0x01

          0x51, 0x10, // SE V1, V1 (should skip)
          0x6A, 0x55, // LD VA, 0x55
          0x50, 0x20, // SE V0, V2 (should skip)
          0x6A, 0x55, // LD VA, 0x55
          0x90, 0x10, // SNE V0, V1 (should skip)
          0x6A, 0x55, // LD VA, 0x55

          0x91, 0x10, // SNE V1, V1 (should not skip)
          0x6B, 0x55, // LD VB, 0x55
          0x90, 0x20, // SNE V0, V2 (should not skip)
          0x6C, 0x55, // LD VC, 0x55
          0x50, 0x10, // SE V0, V1 (should not skip)
          0x6D, 0x55, // LD VD, 0x55
      },
      0, 0);

  // Run until complete
  int i;
  for (i = 0; (i <= 20) && (machine.memory()[machine.pc()] != 0); ++i) {
    ASSERT_EQ(machine.step(0, 0), statemachine::NO_ERROR);
  }

  ASSERT_NE(i, 20); // Make sure we didn't go somewhere we're not meant to be.

  ASSERT_EQ(machine.regs()[0xA], 0); // VA should never have been set.
  // VB, VC, and VD should have been set.
  ASSERT_EQ(machine.regs()[0xB], 0x55);
  ASSERT_EQ(machine.regs()[0xC], 0x55);
  ASSERT_EQ(machine.regs()[0xD], 0x55);
}

TEST(StateMachineTest, Test8xy0) {
  statemachine machine(
      {
          0x61, 0x44, // LD V1, 0x44
          0x83, 0x10, // LD V3, V1
      },
      0, 0);

  ASSERT_EQ(machine.step(0, 0), statemachine::NO_ERROR);
  ASSERT_EQ(machine.step(0, 0), statemachine::NO_ERROR);

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
  statemachine machine(
      {
          0x61, 0x05, // LD V1, 0x05
          0x62, 0xA0, // LD V2, 0xA0

          0x6A, 0x55, // LD VA, 0x55
          0x6B, 0x55, // LD VB, 0x55
          0x6C, 0x55, // LD VC, 0x55
          0x6D, 0x55, // LD VD, 0x55
          0x6E, 0x55, // LD VE, 0x55
          0x6F, 0x55, // LD VF, 0x55

          0x8A, 0x11, // OR VA, V1
          0x8B, 0x21, // OR VB, V2
          0x8C, 0x12, // AND VC, V1
          0x8D, 0x22, // AND VD, V2
          0x8E, 0x13, // XOR VE, V1
          0x8F, 0x23, // XOR VF, V2
      },
      0, 0);

  // Execute all 14 instructions.
  for (int i = 0; i < 14; ++i) {
    ASSERT_EQ(machine.step(0, 0), statemachine::NO_ERROR);
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
  statemachine machine(instruction_decode({
                           0x6082, // LD V0, 0x82
                           0x6102, // LD V1, 0x02
                           0x8104, // ADD V1, V0
                           0x82F0, // LD V2, VF (save VF for later)
                           0x8004, // ADD V0, V0 (should overflow)
                       }),
                       0, 0);

  for (int i = 0; i < 5; ++i) {
    ASSERT_STEP(machine, 0, 0);
  }

  ASSERT_EQ(machine.regs()[0x0], 0x04); // 0x82 + 0x82 = 0x104
  ASSERT_EQ(machine.regs()[0xF], 0x01); // Should have overflowed.
  ASSERT_EQ(machine.regs()[0x1], 0x84); // 0x82 + 0x02 = 0x84
  ASSERT_EQ(machine.regs()[0x2], 0x00); // Should not have overflowed.
}

TEST(StateMachineTest, Test8xy5) {
  statemachine machine(instruction_decode({
                           0x6082, // LD V0, 0x82
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
    ASSERT_STEP(machine, 0, 0);
  }

  ASSERT_EQ(machine.regs()[0xA], 0x80); // 0x82 - 0x02 = 0x80
  ASSERT_EQ(machine.regs()[0xB], 0x01); // 0x82 - 0x02 does not borrow.
  ASSERT_EQ(machine.regs()[0x1], 0x70); // 0x02 - 0x92 = -0x70
  ASSERT_EQ(machine.regs()[0xF], 0x00); // 0x02 - 0x92 DOES borrow.
}

TEST(StateMachineTest, Test8xy6) {
  statemachine machine(instruction_decode({
                           0x6082, // LD V0, 0x82
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
    ASSERT_STEP(machine, 0, 0);
  }

  ASSERT_EQ(machine.regs()[0xA], 0x80); // 0x82 - 0x02 = 0x80
  ASSERT_EQ(machine.regs()[0xB], 0x01); // 0x82 - 0x02 does not borrow.
  ASSERT_EQ(machine.regs()[0x1], 0x70); // 0x02 - 0x92 = -0x70
  ASSERT_EQ(machine.regs()[0xF], 0x00); // 0x02 - 0x92 DOES borrow.
}
