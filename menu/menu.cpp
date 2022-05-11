/*
 *
 * CSEE 4840 Lab 2 for 2022
 *
 * Name/UNI: Elysia Witham (ew2632)
 */
#include "keyboard.hpp"
#include "fpga_ram.h"
#include <iostream>
#include <cstdlib>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

#define GAME1_INSTR 0x00
#define GAME2_INSTR 0x00
#define GAME3_INSTR 0x00
#define GAME4_INSTR 0x00
#define GAME5_INSTR 0x00

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int chip8_fd;

void print_mem(unsigned char addr) {
  fpga_ram_arg_t real_vla;
  real_vla.address = addr;
  real_vla.data = 0x04;
  if (ioctl(chip8_fd, FPGA_RAM_READ, &real_vla)) {
      perror("ioctl(FPGA_RAM_READ) failed");
      return;
  }
  printf("%02x\n",
	real_vla.data);

}

/* Set the background color */
void set_mem(fpga_ram_arg_t *vla)
{
  fpga_ram_arg_t real_vla;
  real_vla.address = vla->address;
  real_vla.data = vla->data;
  if (ioctl(chip8_fd, FPGA_RAM_WRITE, &real_vla)) {
      perror("ioctl(FPGA_RAM_WRITE) failed");
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
  
  /* set up chip 8 userspace */
  static const char filename[] = "";
  std::cout << "CHIP 8 Userspace program started\n";
  fpga_ram_arg_t my_mem_args;
  my_mem_args.data = 0x07;
  my_mem_args.address = 0x07;

  if ( (chip8_fd = open(filename, O_RDWR)) == -1) {
    std::cerr << "could not open file\n";
    return -1;
  }
  memset(&my_mem_args, 0, sizeof(fpga_ram_arg_t));

  /* Look for and handle keypresses */
  for (;;) {
    Keyboard::keys keys = keyboard_obj.get_keys();
    if (keys.game1) {
      std::cout << "Game 1 selected\n";
      print_mem(GAME1_INSTR);
    }
    else if (keys.game2) {
      std::cout << "Game 2 selected\n";
      print_mem(GAME2_INSTR);
    }
    else if (keys.game3) {
      std::cout << "Game 3 selected\n";
      print_mem(GAME3_INSTR);
    }
    else if (keys.game4) {
      std::cout << "Game 4 selected\n";
      print_mem(GAME4_INSTR);
    }
    else if (keys.game5) {
      std::cout << "Game 5 selected\n";
      print_mem(GAME5_INSTR);
    }
    std::cout << (int)keys.keypad << std::endl;
  }

  return 0;
}

