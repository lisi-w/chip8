#include "keyboard.hpp"
#include <cstdlib>
#include <iostream>

#define ENTER 0x28
#define ESC 0x29

#define LEFT 0x50
#define RIGHT 0x4F

uint8_t endpoint_address;

int main() {

  /* Open the keyboard */
  Keyboard keyboard_obj;
  if (!keyboard_obj.find_keyboard()) {
    std::cerr << "Did not find a keyboard\n";
    exit(1);
  }
  std::cout << "Keyboard initialized\n";

  /* Look for and handle keypresses */
  for (;;) {
    Keyboard::keys keys = keyboard_obj.get_keys();
    std::cout << (int)keys.keypad << std::endl;
  }

  return 0;
}
