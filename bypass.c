//Support S0381 and S0211
//GPIO controled by ASpeed AST2300 and AST1300

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#include <sitest.h>
#include <libsio.h>

#define EFER 0x2e
#define EFDR 0x2f

#define Low_GPIO_BASE_ADDR 0x0000 /* Refer to AST Datasheet page 416 */
#define High_GPIO_BASE_ADDR 0x1E78		

#define uchar unsigned char


void usage(void);
void set_bp(int pair, int on,int off, int wdt);

uchar read_sio(int index);
void write_sio(int index, uchar val_w);
uchar read_reg(int lr, int hr);
void write_reg(uchar val_w, int lw, int hw);
int check_arguments(char *argv[], int *, int *, int *, int *);

int main(int argc, char *argv[])
{
	uchar b;
	int pair, on, off, wdt;

	if (argc < 5) {
		usage();
		exit(-1);
	}

	if (check_arguments(argv, &pair, &on, &off, &wdt)) {
		ERR("Incorrect arguments!\n");
		usage();
		exit(-1);
	}

	DBG("pair = %d, on = %d, off = %d, wdt = %d\n", pair, on, off, wdt);
	exit(0);

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	sio_enter("AST1300");

	set_bp(pair, on, off, wdt);

	sio_exit();

	return 0;
}

int check_arguments(char *argv[], int *pair, int *on, int *off, int *wdt)
{
	int i;

	for (i = 1; i <= 4; i++) {
		DBG("argv[%d] = %s\n", i, argv[i]);
		if (atoi(argv[i]) < 0 || atoi(argv[i]) > 1)
			return 1;
	}

	*pair = atoi(argv[1]);
	*on = atoi(argv[2]);
	*off = atoi(argv[3]);
	*wdt = atoi(argv[4]);

	return 0;
}

uchar read_sio(int index)
{
	uchar b;

	outb_p (index, EFER);		//Sent index to EFER
	b = inb_p (EFDR);			//Get the value from EFDR
	return b;					//Return the value
}

void write_sio(int index, uchar val_w)
{
	outb_p (index, EFER);			//Sent index at EFER
	outb_p (val_w, EFDR);			//Send val_w at EFDR
}

uchar read_reg(int lr, int hr)
{
	uchar b;
	int mod;

	write_sio (0x07, 0x0d);	//Set Logical device number to SIORD (iLPC2AHB)

	//Enable SIO iLPC2AHB
	b=read_sio (0x30);
	b |= 0x01;
	write_sio (0x30, b);

	//Set Length to 1 Byte
	b=read_sio (0xf8);
	b &= ~(0x03);
	write_sio (0xf8, b);

	mod = lr%4;		//Address must be multiple of 4
	lr = lr - mod;

	b = hr>>8;		//Set address
	write_sio (0xf0, b);
	b = hr&0x00ff;
	write_sio (0xf1, b);
	b = lr>>8;
	write_sio (0xf2, b);
	b = lr&0x00ff;
	write_sio (0xf3, b);

	read_sio(0xfe);		//Read Trigger
	usleep (10000);

	b = read_sio (0xf7 - mod);	//Get the value
	return b;
}

void write_reg(uchar val_w, int lw, int hw)
{
	int b;
	int mod;

	write_sio (0x07, 0x0d);		//Set Logical device number to SIORD

	//Enable SIO iLPC2AHB
	b=read_sio (0x30);
	b |= 0x01;
	write_sio (0x30, b);

	//Set Length to 1 Byte
	b=read_sio (0xf8);
	b &= ~(0x03);
	write_sio (0xf8, b);

	//	mod = lw%4;		//Address must be multiple of 4
	//	lw = lw - mod;

	b = hw>>8;						//Set address
	write_sio (0xf0, b);
	b = hw&0x00ff;
	write_sio (0xf1, b);
	b = lw>>8;
	write_sio (0xf2, b);
	b = lw&0x00ff;
	write_sio (0xf3, b);

	//read_sio(0xfe);		//Read Trigger
	//usleep (10000);

	write_sio (0xf7, val_w);	//Send the value

	write_sio (0xfe, 0xcf);		//Write Trigger
	usleep (10000);
}

void set_bp(int pair, int on, int off, int wdt)
{
	int procedure = 4;
	uchar data;

	/* Set GPIOJ0, J1, J2, J3 to output */
	data = read_reg(Low_GPIO_BASE_ADDR + 0x75, High_GPIO_BASE_ADDR);
	data |= 0x0f;
	write_reg(data, Low_GPIO_BASE_ADDR + 0x75, High_GPIO_BASE_ADDR);

	/* Set GPIOG0, G1, G2 to output */
	data = read_reg(Low_GPIO_BASE_ADDR + 0x26, High_GPIO_BASE_ADDR);
	data |= 0x07;
	write_reg(data, Low_GPIO_BASE_ADDR + 0x26, High_GPIO_BASE_ADDR);

	// Set Pair (GPIOJ0, J1, J2)
	printf ("Set Pair to %02X\n", pair);
	data = 	read_reg(Low_GPIO_BASE_ADDR + 0x71, High_GPIO_BASE_ADDR);
	data &= ~(0x07);
	data |= pair;
	write_reg(data, Low_GPIO_BASE_ADDR + 0x71, High_GPIO_BASE_ADDR);

	//Set CFG1 (GPIOJ3)
	data = 	read_reg(Low_GPIO_BASE_ADDR + 0x71, High_GPIO_BASE_ADDR);
	printf ("Set CFG1 from %02X", data);
	data &= ~(0x08);
	data |= wdt<<3;
	printf (" to %02X\n", data);
	write_reg(data, Low_GPIO_BASE_ADDR + 0x71, High_GPIO_BASE_ADDR);

	//Set CFG2 (GPIOG0)
	data = 	read_reg(Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);
	printf ("Set CFG2 from %02X", data);
	data &= ~(0x01);
	data |= off;
	printf (" to %02X\n", data);
	write_reg(data, Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);

	//Set CFG3 (GPIOG1)
	data = 	read_reg(Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);
	printf ("Set CFG3 from %02X", data);
	data &= ~(0x02);
	data |= on<<1;
	printf (" to %02X\n", data);
	write_reg(data, Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);

	//Set Sendbits to high then low (GPIOG2)
	data = 	read_reg(Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);
	printf ("Set Sendbit from %02X", data);
	data |= (0x04);
	printf (" to %02X\n", data);
	write_reg(data, Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);

	usleep (200000);

	data = 	read_reg(Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);
	printf ("Set Sendbit from %02X", data);
	data &= ~(0x04);
	printf (" to %02X\n", data);
	write_reg(data, Low_GPIO_BASE_ADDR + 0x22, High_GPIO_BASE_ADDR);
}

void usage(void)	//If parameter is wrong, it will show this message
{
	printf("Support S0381 and S0211\n"
			"Usage : Command [Pair] [PWRON] [PWROFF] [WDT]\n"
			" Pair : 0 and 1 for Slot1\n"
			"        2 and 3 for Slot2\n"
			"        4 and 5 for Slot3\n"
			"        6 and 7 for Slot4\n"
			" PWRON Status : 0(Passthru), 1(Bypass)\n"
			" PWROFF Status : 0(Passthru), 1(Bypass)\n"
			" WDT trigger : 0(reset), 1(bypass)\n");
}
