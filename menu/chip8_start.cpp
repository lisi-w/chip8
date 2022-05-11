/*
 *
 * CSEE 4840 Lab 2 for 2022
 *
 * Name/UNI: Elysia Witham (ew2632)
 */
#include "keyboard.hpp"
#include "chip8.h"
#include <iostream>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#define PONG_INSTR 0x00
#define TETRIS_INSTR 0x00
#define SPACE_INVADERS_INSTR 0x00
#define SOCCER_INSTR 0x00
#define CAVE_INSTR 0x00
#define START_SCREEN 0x00

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int chip8_fd;

void send_instr(unsigned int instr)
{
  if (ioctl(chip8_fd, CHIP8_INSTR_WRITE, &instr)) {
      perror("ioctl(CHIP8_INSTR_WRITE) failed");
      return;
  }
}

int main()
{

  /* Open the keyboard */
  Keyboard keyboard_obj;
  if (!keyboard_obj.find_keyboard()) {
    std::cerr << "Did not find a keyboard\n";
    exit(1);
  }
  std::cout << "Keyboard initialized\n";
  unsigned int chip8_instr;
  int in_game;
  
  /* set up chip 8 userspace */
  static const char filename[] = "/dev/chip8";
  std::cout << "CHIP8 Userspace program started\n";

  if ( (chip8_fd = open(filename, O_RDWR)) == -1) {
    std::cerr << "could not open file\n";
    return -1;
  }

  /* Look for and handle keypresses */
  for (;;) {
    Keyboard::keys keys = keyboard_obj.get_keys();
    if (in_game) {
      if (keys.escape) {
        in_game = 0;
        continue;
      }
      std::cout << (int)keys.keypad << std::endl;
      send_instr(keys.keypad);
    }
    else if (keys.game1) {
      std::cout << "Game 1 selected\n";
      send_instr(PONG_INSTR);
      in_game = 1;
    }
    else if (keys.game2) {
      std::cout << "Game 2 selected\n";
      send_instr(TETRIS_INSTR);
      in_game = 1;
    }
    else if (keys.game3) {
      std::cout << "Game 3 selected\n";
      send_instr(SPACE_INVADERS_INSTR);
      in_game = 1;
    }
    else if (keys.game4) {
      std::cout << "Game 4 selected\n";
      send_instr(SOCCER_INSTR);
      in_game = 1;
    }
    else if (keys.game5) {
      std::cout << "Game 5 selected\n";
      send_instr(CAVE_INSTR);
      in_game = 1;
    }
    else {
      send_instr(START_SCREEN);
    }
  }

  return 0;
}

