#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#include <sitest.h>
#include <libsio.h>

#define LOW_DATA_BASE_ADDR   0x2000
#define HIGH_DATA_BASE_ADDR  0x1E72

#define LOW_PECI_BASE_ADDR   0x1C00
#define HIGH_PECI_BASE_ADDR  0x1E72

void init_peci(void);

int main(void)
{
	char chip_model[10] = "AST1300";
	unsigned long int data;

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	EFER = 0x2E;
	EFDR = EFER + 1;

	printf("chip: %s, EFER: %x, EFDR: %x\n", chip_model, EFER, EFDR);

	sio_enter(chip_model);
	init_peci();
	data = sio_ilpc2ahb_readl(LOW_DATA_BASE_ADDR + 0x01C0, HIGH_DATA_BASE_ADDR);
	printf("Firmware Ver: %x\n", data);
	data = sio_ilpc2ahb_readl(LOW_DATA_BASE_ADDR + 0x01C4, HIGH_DATA_BASE_ADDR);
	printf("OEM Ver: %x\n", data);

	sio_exit();

	return 0;
}

void init_peci(void)
{
	unsigned int data;

	/* Enable PECI, Negotiation Timing = 0x40, Clock divider = 2 */
	sio_ilpc2ahb_write(0x02, LOW_PECI_BASE_ADDR + 0x00, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x40, LOW_PECI_BASE_ADDR + 0x01, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x02, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x03, HIGH_PECI_BASE_ADDR);

	/* Read length = 2, Write length = 1, CPU address = 0x30 */
	sio_ilpc2ahb_write(0x30, LOW_PECI_BASE_ADDR + 0x04, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x05, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x02, LOW_PECI_BASE_ADDR + 0x06, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x07, HIGH_PECI_BASE_ADDR);

	/* Write Command Code (0x01) into write Register */
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x0C, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x0D, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x0E, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x0F, HIGH_PECI_BASE_ADDR);

	/* Fire Engine */
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x08, HIGH_PECI_BASE_ADDR);

	/* Check the status is Idle or Busy */
	data = 2;
	while (data != 0) {
		usleep(10000);
		data = sio_ilpc2ahb_read(LOW_PECI_BASE_ADDR + 0x08, HIGH_PECI_BASE_ADDR);
		data &= 0x02;
	}
}

