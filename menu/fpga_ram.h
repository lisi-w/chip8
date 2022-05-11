#ifndef _FPGA_RAM_H
#define _FPGA_RAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ioctl.h>

#define FPGA_RAM_MAGIC 'q'

typedef struct {
	unsigned int address;
	unsigned int data;
	unsigned int readdata;
} fpga_ram_arg_t;

/* ioctls and their arguments */
#define FPGA_RAM_WRITE _IOW(FPGA_RAM_MAGIC, 1, fpga_ram_arg_t *)
#define FPGA_RAM_READ  _IOR(FPGA_RAM_MAGIC, 2, fpga_ram_arg_t *)
#define FPGA_RAM_READ_L  _IOR(FPGA_RAM_MAGIC, 3, fpga_ram_arg_t *)
#define FPGA_RAM_WRITE_L  _IOR(FPGA_RAM_MAGIC, 4, fpga_ram_arg_t *)

#ifdef __cplusplus
}
#endif

#endif
