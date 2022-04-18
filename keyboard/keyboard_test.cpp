#include <chrono>
#include <iostream>
#include <thread>

#include "keyboard.hpp"

/* This is some pseudo code that should, in theory, find a keyboard
 * and every half a second check if the user has pressed "enter".
 */
int main() {
  Keyboard keyboard;

  std::cout << "Trying to find keyboard!\n";

  if (!keyboard.find_keyboard()) {
    std::cout << "Could not find a keyboard! Yeeting!" << std::endl;
    return 1;
  }

  std::cout << "Found a keyboard!\n";
  std::cout << "Waiting for you to press enter!\n";

  // An example of something a driver might ask for.
  while (true) {
    Keyboard::keys keys = keyboard.get_keys();
    if (keys.error != SUCCESS) {
      std::cout << "Polling failed!" << std::endl;
      return 1;
    }

    if (keys.enter) {
      std::cout << "Detected enter key press!\n";
      return 0;
    }

    std::cout << "Key not detected. Trying again!\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}
