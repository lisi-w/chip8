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
#include <unistd.h>
#include <cstring>
#include <csignal>

#define MEMORY_START 0x200
#define MEMORY_END 0x1000

static int CHIP8_FONTSET[] = 
	{
		0xF0, 0x90, 0x90, 0x90, 0xF0, //0
		0x20, 0x60, 0x20, 0x20, 0x70, //1
		0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
		0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
		0x90, 0x90, 0xF0, 0x10, 0x10, //4
		0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
		0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
		0xF0, 0x10, 0x20, 0x40, 0x40, //7
		0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
		0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
		0xF0, 0x90, 0xF0, 0x90, 0x90, //A
		0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
		0xF0, 0x80, 0x80, 0x80, 0xF0, //C
		0xE0, 0x90, 0x90, 0x90, 0xE0, //D
		0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
		0xF0, 0x80, 0xF0, 0x80, 0x80  //F
	};

/*
 * References:
 *
 * http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html
 * http://www.thegeekstuff.com/2011/12/c-socket-programming/
 * 
 */

int chip8_fd;

void close_prog(int signal) {
  close(chip8_fd);
  exit(0);
}

void send_opcode(opcode *op)
{
  if (ioctl(chip8_fd, CHIP8_INSTR_WRITE, op)) {
      perror("ioctl(CHIP8_INSTR_WRITE) failed");
      close_prog(-1);
  }
}

void set_mem(int address, int data) {
  opcode op;
  op.addr = MEMORY_ADDR;
  op.data = (1 << 20) | ((address & 0xfff) << 8) | (data & 0xff);
  send_opcode(&op);	
}

void load_fonts() {
  for (int i = 0; i < 80; ++i) {
    set_mem(i, CHIP8_FONTSET[i]);
  }
}

void reset_mem() {
  for (int i = 0; i < MEMORY_END; i++) {
    set_mem(i, 0);
  }
}

void load_rom(const char *game) {
  FILE *gamefile;
  char buf[4];
  long filelen;
  long sz = MEMORY_END - MEMORY_START;
  int i;
  int val;
  
  gamefile = fopen(game, "r");
  fseek(gamefile, 0, SEEK_END);
  filelen = ftell(gamefile);
  rewind(gamefile);

  for (i = 0; i < filelen && i < sz; i++) {
    fread((&buf), 1, 2, gamefile);
    // convert buf to hex
    val = (int)strtol(buf, NULL, 16);
    set_mem(MEMORY_START + i, val);
  }

  for (int j = i; j < sz; ++j) {
    set_mem(MEMORY_START + j, 0);
  }
}

void send_input(char value) {
  opcode op;
  op.addr = KEY_PRESS_ADDR;
  op.data = ((1 & 0x1) << 4) | (value & 0xf);
  send_opcode(&op);
}

void reset_all() {
  opcode fb_op;
  opcode pc_op;
  opcode wr_op;
  opcode ir_op;
  opcode stk_op;
  opcode st_op;
  opcode dt_op;
  reset_mem();
  load_fonts();
  for (int i = 0; i < 64; ++i) {
    for (int j = 0; j < 32; ++j) {
      fb_op.addr = FRAMEBUFFER_ADDR;
      fb_op.data = (1 << 12) | ((0 & 0x1) << 11) | ((i & 0x3f) << 5) | (j & 0x1f);
      send_opcode(&fb_op);
    }
  }
  for (int k = 0; k < 0x10; ++k) {
    wr_op.addr = V0_ADDR + 4 * (k & 0xf);
    wr_op.data = 0 & 0xff;
    send_opcode(&wr_op);
  }
  pc_op.addr = PROGRAM_COUNTER_ADDR;
  pc_op.data = 0x200;
  send_opcode(&pc_op);
  ir_op.addr = I_ADDR;
  ir_op.data = 0 & 0xffff;
  send_opcode(&ir_op);
  stk_op.addr = STACK_ADDR;
  send_opcode(&stk_op);
  st_op.addr = SOUND_TIMER_ADDR;
  st_op.data = 0;
  send_opcode(&st_op);
  dt_op.addr = DELAY_TIMER_ADDR;
  dt_op.data = 0;
  send_opcode(&dt_op);
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
    std::cerr << "could not open /dev/chip8\n";
    return -1;
  }

  signal(SIGINT, close_prog);
  reset_all();
  load_rom("start.hex");
  /* Look for and handle keypresses */
  for (;;) {
    Keyboard::keys keys = keyboard_obj.get_keys();
    if (in_game) {
      if (keys.escape) {
        in_game = 0;
        reset_all();
        load_rom("start.hex");
        continue;
      }
      for (int i = 0; i < 6; i++) {
        if (keys.keycode[i]) 
          send_input(keys.keycode[i]);
      }
    }
    else if (keys.game1) {
      std::cout << "Pong selected\n";
      load_rom("pong.hex");
      in_game = 1;
    }
    else if (keys.game2) {
      std::cout << "Tetris selected\n";
      load_rom("tetris.hex");
      in_game = 1;
    }
    else if (keys.game3) {
      std::cout << "Space Invaders selected\n";
      load_rom("space_invaders.hex");
      in_game = 1;
    }
    else if (keys.game4) {
      std::cout << "Soccer selected\n";
      load_rom("soccer.hex");
      in_game = 1;
    }
    else if (keys.game5) {
      std::cout << "Cave selected\n";
      load_rom("cave.hex");
      in_game = 1;
    }
  }

  return 0;
}

