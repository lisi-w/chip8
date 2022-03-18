#include "statemachine.hpp"
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
