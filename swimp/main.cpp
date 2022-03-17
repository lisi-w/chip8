#include <iostream>

#include "statemachine.hpp"

int main(const int, const char **) {
  using namespace std;
  statemachine machine(statemachine::instructions(), 0);
  cout << "Hello World!\n";
  return 0;
}

