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

int main(void)
{
	int data;
	unsigned int base_addr;

	EFER = 0x2E;
	EFDR = 0x2F;

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	/* sio enter */
	outb_p(0x55, EFER);

	/* read device id */
	data = sio_read(0x20);
	printf("device id = %x\n", data);

	/* select logic device number 0xA for hwmon */
	sio_select(0x0A);

	/* get hwmon base addr */
	base_addr = (sio_read(0x60) << 8);
	base_addr |= sio_read(0x61);
	printf("hwmon base addr = %x\n", base_addr);
	base_addr += 0x70;

	/* read hwmon status */
	outl_p(0x40, base_addr);
	data = inl_p(base_addr + 1);
	printf("0x40 = %x\n", data);

	/* sio exit */
	outb_p(0xAA, EFER);

	return 0;
}
