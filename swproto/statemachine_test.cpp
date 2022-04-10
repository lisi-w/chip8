#include "statemachine.hpp"
#include <algorithm>
#include <gtest/gtest.h>

// Convenience function for debugging.
void print_regs(const statemachine &mach) {
  using namespace std;
  cerr << "Regs: ";
  for (uint8_t reg : mach.regs()) {
    cerr << hex << setw(2) << setfill('0') << (int)reg << " ";
  }
  cerr << endl;
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
