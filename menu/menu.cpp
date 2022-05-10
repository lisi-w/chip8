/*
 *
 * CSEE 4840 Lab 2 for 2022
 *
 * Name/UNI: Elysia Witham (ew2632)
 */
#include "fbputchar.h"
#include "keyboard.hpp"
#include <iostream>
#include <cstdlib>

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */


void clear_screen() {
  for (int col = 0; col < 64; col++) {
    for (int row = 0; row < 24; row++) {
      fbputchar(' ', row, col);
    }
  }
}

int main()
{
  int err, col;

  if ((err = fbopen()) != 0) {
    std::cerr << "Error: Could not open framebuffer\n";
    exit(1);
  }

  clear_screen();
  fbputs("1. game1", 1, 5);
  fbputs("2. game2", 2, 5);
  fbputs("3. game3", 3, 5);
  fbputs("4. game4", 4, 5);
  fbputs("5. game5", 5, 5);

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
    if (keys.game1) {
      clear_screen();
      fbputs("Game 1 Selected", 1, 5);
    }
    else if (keys.game2) {
      clear_screen();
      fbputs("Game 2 Selected", 1, 5);
    }
    else if (keys.game3) {
      clear_screen();
      fbputs("Game 3 Selected", 1, 5);
    }
    else if (keys.game4) {
      clear_screen();
      fbputs("Game 4 Selected", 1, 5);
    }
    else if (keys.game5) {
      clear_screen();
      fbputs("Game 5 Selected", 1, 5);
    }
    std::cout << (int)keys.keypad << std::endl;
  }

  return 0;
}

