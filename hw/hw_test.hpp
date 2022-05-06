#ifndef YACHIP8_HW_HW_TEST_HPp
#define YACHIP8_HW_HW_TEST_HPp
#include <algorithm>
#include <any>
#include <bitset>
#include <functional>
#include <gtest/gtest.h>
#include <initializer_list>
#include <memory>
#include <sstream>
#include <verilated_vcd_c.h>

static const uint64_t PICOS = 1000000000000ul;
static const uint64_t HZ = 50000000ul;
static const uint64_t HALF_CYCLE_TIME = PICOS / HZ / 2;

extern const std::unique_ptr<VerilatedContext> contextp;
// dut is short for Device Under Test
template <typename VModel> class VTest : public ::testing::Test {
protected:
  // Device under test.
  VModel dut;
  // Trace file
  VerilatedVcdC tf;
  uint64_t time;
  const testing::TestInfo *const test_info =
      testing::UnitTest::GetInstance()->current_test_info();

  std::string get_tf_name() {
    std::stringstream name;
    name << TRACE_DIR << test_info->test_suite_name() << '_'
         << test_info->name() << ".vcd";
    return name.str();
  }

  VTest() : dut{}, tf{}, time{0} {
    dut.final();
    dut.trace(&tf, 99);
    auto tf_name = get_tf_name();
    tf.open(tf_name.c_str());
    if (!tf.isOpen()) {
      std::cerr << "Failed to open " + tf_name + "; aborting.\n";
      abort();
      return;
    }
  }

  auto &&dut_clk_member();

  inline void eval() { dut.eval(); }
  inline void tick() {
    dut_clk_member() = 1;
    dut.eval();
    tf.dump(time);
    time += HALF_CYCLE_TIME;
  }
  inline void tock() {
    dut_clk_member() = 0;
    dut.eval();
    tf.dump(time);
    time += HALF_CYCLE_TIME;
  }

  inline void ticktock() {
    tick();
    tock();
  }

  ~VTest() {
    dut.final();
    if (tf.isOpen()) {
      tf.close();
    }
  }
};

template <typename VModel> auto &&VTest<VModel>::dut_clk_member() {
  return dut.clk;
}

#endif // YACHIP8_HW_HW_TEST_HPp
