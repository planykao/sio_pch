/*
 * This program can monitor the Watchdog event counter of AST1300/AST2300.
 * When the timeout event happened, it will calculate how long does the 
 * timeout event happened.
 * Usage: ./count #source_clock(0: PCLK, 1: 1MHz) #time
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include <sitest.h>
#include <libsio.h>

#define Low_WDT_BASE_ADDR 0x5000		// Base address of Watchdog Timer. Refer to AST Datasheet page 472
#define High_WDT_BASE_ADDR 0x1E78		

#define Low_GPIO_BASE_ADDR 0x0000		// Base address of GPIO. Refer to AST Datasheet page 472
#define High_GPIO_BASE_ADDR 0x1E78		

#define Low_SCU_BASE_ADDR 0x2000		// Base address of System Control Unit (SCU). Refer to AST Datasheet page 308
#define High_SCU_BASE_ADDR 0x1E6E

#define AST_WDT_RESTART 0x4755
#define AST_SCU_UNLOCK  0x1688A8A8
#define PCLK  24000000
#define MHz  1000000 

struct timeval start, end;

void read_counter()
{
	unsigned long int data;

	data = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR, High_WDT_BASE_ADDR);
	data |= (sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 1, High_WDT_BASE_ADDR) << 8);
	data |= (sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 2, High_WDT_BASE_ADDR) << 16);
	data |= (sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 3, High_WDT_BASE_ADDR) << 24);

	printf("Counter status = %x\n", data);
}

void read_reload()
{
	unsigned long int data;

	data = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x4, High_WDT_BASE_ADDR);
	data |= (sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x5, High_WDT_BASE_ADDR) << 8);
	data |= (sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x6, High_WDT_BASE_ADDR) << 16);
	data |= (sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x7, High_WDT_BASE_ADDR) << 24);

	printf("Reload = %x\n", data);
}

void monitor()
{
	unsigned long int pre, cur;
	double pre_time, cur_time;

	cur = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x11, High_WDT_BASE_ADDR);
	gettimeofday(&start, NULL);
	pre_time = cur_time = ((double)(start.tv_sec * 1000000) + \
                           (double)start.tv_usec) / 1000000;

	while (1) {
		pre = cur;
		cur = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x11, High_WDT_BASE_ADDR);
		
		if (pre != cur) {
			gettimeofday(&start, NULL);
			cur_time = ((double)(start.tv_sec * 1000000) + \
                        (double)start.tv_usec) / 1000000;
			printf("time: %f\n", cur_time - pre_time);
			pre_time = cur_time;
		}
	}
}

int main(int argc, char *argv[])
{
	unsigned long int value, reload;
	
	int addr_low, src_clk;
	unsigned int data;

	if (argc != 3) {
		printf("Usage: ./count #source_clock(0: PCLK, 1: 1MHz) #time\n");
		exit(1);
	}

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	if (atoi(argv[1]))
		value = MHz * atoi(argv[2]);
	else
		value = PCLK * atoi(argv[2]);

	EFER = 0x2E;
	EFDR = EFER + 1;

	sio_enter("AST1300");

 	data = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x0C, High_WDT_BASE_ADDR);
 	data &= ~(0x1 << 0);
 	sio_ilpc2ahb_write(data, Low_WDT_BASE_ADDR + 0x0C, High_WDT_BASE_ADDR);
 	printf("Disable WDT\n");

	/* Clear WDT1 Timeout Register */
 	addr_low = Low_WDT_BASE_ADDR + 0x14;
 	sio_ilpc2ahb_write(0x3b, addr_low++, High_WDT_BASE_ADDR);
 	sio_ilpc2ahb_write(0x00, addr_low++, High_WDT_BASE_ADDR);
 	sio_ilpc2ahb_write(0x00, addr_low++, High_WDT_BASE_ADDR);
 	sio_ilpc2ahb_write(0x00, addr_low, High_WDT_BASE_ADDR);
 	printf("Clear Timeout register\n");

	/* Set WDT1 Counter Reload Value */
 	addr_low = Low_WDT_BASE_ADDR + 0x04;
	sio_ilpc2ahb_writel(value, addr_low, High_WDT_BASE_ADDR);
 	printf("Set Reload value\n");

	/* Write 0x4755 to WDT1 Counter Restart Register */
 	addr_low = Low_WDT_BASE_ADDR + 0x08;
	sio_ilpc2ahb_writel(AST_WDT_RESTART, addr_low, High_WDT_BASE_ADDR);
 	printf("Restart register\n");

	/* Clock Select for WDT1 Counter, WDT0C[4], 0: PCLK, 1: 1MHz */
	addr_low = Low_WDT_BASE_ADDR + 0x0c; // PCLK
 	data = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x0C, High_WDT_BASE_ADDR);
 	data &= ~(0x1 << 4);
	data |= (atoi(argv[1]) << 4);
 	sio_ilpc2ahb_write(data, Low_WDT_BASE_ADDR + 0x0C, High_WDT_BASE_ADDR); 	

	/* Clear timeout and boot code selection status */
 	data = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x14, High_WDT_BASE_ADDR);
 	data = 0x01;
 	sio_ilpc2ahb_write(data, Low_WDT_BASE_ADDR + 0x14, High_WDT_BASE_ADDR); 
 
	addr_low = Low_SCU_BASE_ADDR + 0x84;						//Enable Enable WDT out putfunction pin in SCU[84] D[4]/D[5]
 	data = sio_ilpc2ahb_read(addr_low, High_SCU_BASE_ADDR);
 	data |= 0x10;
 	sio_ilpc2ahb_write(data, addr_low, High_SCU_BASE_ADDR);
 	addr_low++;
 	data = sio_ilpc2ahb_read(addr_low, High_SCU_BASE_ADDR);
 	data |= 0xf0;
 	sio_ilpc2ahb_write(data, addr_low, High_SCU_BASE_ADDR);

	/* Enable Watchdog timer */
 	data = sio_ilpc2ahb_read(Low_WDT_BASE_ADDR + 0x0c, High_WDT_BASE_ADDR);
 	data |= 0xd;
	sio_ilpc2ahb_write(data, Low_WDT_BASE_ADDR + 0x0c, High_WDT_BASE_ADDR);
	printf("Enable WDT\n");

	monitor();

	sio_exit();

	return 0;
}

