#include "Vmymodule.h"
#include "hw_test.hpp"

using MymoduleTest = VTest<Vmymodule>;

TEST_F(MymoduleTest, counts) {
  dut.do_count = true;
  ticktock();
  ASSERT_EQ(dut.counter, 0);
  ticktock();
  ASSERT_EQ(dut.counter, 1);
  dut.do_count = false;
  ASSERT_EQ(dut.counter, 1);
  dut.do_count = true;
  ticktock();
  ASSERT_EQ(dut.counter, 2);
  ticktock();
  ASSERT_EQ(dut.counter, 3);
  ticktock();
  ASSERT_EQ(dut.counter, 0);
}
