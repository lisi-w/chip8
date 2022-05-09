#include "hw_test.hpp"

#include "Vdisplay_ram.h"

using DisplayRamTest = VTest<Vdisplay_ram>;

TEST_F(DisplayRamTest, TestSimpleWriteRead) {
  eval();
  dut.wa = 1;
  dut.wb = 1;
  // Write mem[0x01] = 0x23; mem[0x45] = 0x67;
  dut.aa = 0x01;
  dut.da = 0x23;
  dut.ab = 0x45;
  dut.db = 0x67;
  dut.wa = 1;
  dut.wb = 1;
  ticktock();
  ASSERT_EQ(dut.qa, 0x23);
  ASSERT_EQ(dut.qb, 0x67);

  // Make sure it does nothing with write enable off on port a.
  dut.wa = 0;
  dut.da = 0xff;
  // Write mem[0x89] = 0xAB;
  dut.ab = 0x89;
  dut.db = 0xAB;
  ticktock();
  ASSERT_EQ(dut.qa, 0x23);
  ASSERT_EQ(dut.qb, 0xAB);

  // Read address 0x01 with port b.
  dut.wb = 0;
  dut.ab = 0x01;
  ticktock();
  ASSERT_EQ(dut.qa, 0x23);
  ASSERT_EQ(dut.qb, 0x23);
}
