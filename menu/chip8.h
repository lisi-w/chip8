#ifndef _CHIP8_H
#define _CHIP8_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>

#define CHIP8_MAGIC 'q'

typedef struct {
  unsigned int data;
  unsigned int addr;
} opcode;

/* ioctls and their arguments */
#define CHIP8_INSTR_WRITE _IOW(CHIP8_MAGIC, 1, opcode *)

#define V0_ADDR 0x00
#define I_ADDR 0x40
#define SOUND_TIMER_ADDR 0x44
#define DELAY_TIMER_ADDR 0x48
#define STACK_POINTER_ADDR 0x60
#define STACK_ADDR 0x4C
#define PROGRAM_COUNTER_ADDR 0x50
#define KEY_PRESS_ADDR 0x54
#define MEMORY_ADDR 0x64
#define FRAMEBUFFER_ADDR 0x5C
#define INSTRUCTION_ADDR 0x68
#define RESET_ADDR 0x6C

#ifdef __cplusplus
}
#endif

#endif
