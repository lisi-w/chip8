#include "hw_test.hpp"

const std::unique_ptr<VerilatedContext> contextp =
    std::make_unique<VerilatedContext>();

int main(int argc, char **argv) {
  contextp->commandArgs(argc, argv);
  contextp->traceEverOn(true);
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
