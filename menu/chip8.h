#ifndef _CHIP8_H
#define _CHIP8_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>

#define CHIP8_MAGIC 'q'

/* ioctls and their arguments */
#define CHIP8_INSTR_WRITE _IOW(CHIP8_MAGIC, 1, unsigned int *)

#ifdef __cplusplus
}
#endif

#endif
