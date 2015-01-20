#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sitest.h>

#define EC_LDM 0x0B
#define NCT6683D_EC_EN_REG   0x30
#define NCT6683D_EC_BASE_REG 0x60
#define NCT6683D_PAGE(n)                (n)
#define NCT6683D_TEMP_VOL_READ_INDEX(n) (0x00 + 2 * n)
#define NCT6683D_TACHO_READ_INDEX(n)    (0x40 + 2 * n)

extern void sio_enter(char *chip);
extern void sio_exit(void);
extern int sio_read(int reg);
extern void sio_write(int reg, int val);
extern void sio_select(int ldnum);
extern void sio_logical_device_enable(int bit);
extern unsigned int EFER;
extern unsigned int EFDR;

/*
 * ec_base_addr, EC space base address.
 * page_p , page port.
 * index_p, index port.
 * data_p , data port.
 */
unsigned int ec_base_addr, page_p, index_p, data_p;

unsigned int read_page_port(unsigned int page_p);
void nct6683d_ec_init(void);
int nct6683d_ec_read(int page, int index);
void nct6683d_ec_write(int page, int index, int data);
unsigned int nct6683d_read_word(int page, int index);
unsigned int nct6683d_read_byte(int page, int index);
unsigned int nct6683d_temp_tacho_read(int page, int index);
unsigned int nct6683d_vin_read(int page, int index);

unsigned int read_page_port(unsigned int page_p)
{
	return inb(page_p);
}

/*
 * setup page port, index port and data port.
 * the ports locate at ec_base_addr +0, +1 and +2 are for BIOS.
 */
void nct6683d_ec_init(void)
{
	page_p  = ec_base_addr + 4;
	index_p = ec_base_addr + 5;
	data_p  = ec_base_addr + 6;
}

int nct6683d_ec_read(int page, int index)
{
	int data;

	/* check page port */
	if (read_page_port(page_p) != 0xFF)
		outb(0xFF, page_p);

	outb(page, page_p);
	outb(index, index_p);
	data = inb(data_p);
	outb(0xFF, page_p);
	
	return data;
}

void nct6683d_ec_write(int page, int index, int data)
{
	/* check page port */
	if (read_page_port(page_p) != 0xFF)
		outb(0xFF, page_p);

	outb(page, page_p);
	outb(index, index_p);
	outb(data, data_p);
	outb(0xFF, page_p);
}

unsigned int nct6683d_read_word(int page, int index)
{
	return (nct6683d_ec_read(page, index) << 8 | \
			nct6683d_ec_read(page, index + 1));
}

unsigned int nct6683d_read_byte(int page, int index)
{
	return (nct6683d_ec_read(page, index));
}

unsigned int nct6683d_temp_tacho_read(int page, int index)
{
	return nct6683d_read_word(page, index);
}

unsigned int nct6683d_vin_read(int page, int index)
{
	return nct6683d_read_byte(page, index);
}

int main(void)
{
	unsigned int data;
	int i;

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	EFER = 0x2E;
	EFDR = EFER + 1;

	sio_enter("NCT6683D");

	printf("device id: %x\n", (sio_read(0x20) << 8) | sio_read(0x21));

	sio_select(EC_LDM);

	ec_base_addr = (sio_read(NCT6683D_EC_BASE_REG) << 8) | \
					sio_read(NCT6683D_EC_BASE_REG + 1);
	printf("EC base address: %x\n", ec_base_addr);
	
	/* setup page port, index port and data port. */
	nct6683d_ec_init();

	sio_logical_device_enable(0);

	data = nct6683d_temp_tacho_read(NCT6683D_PAGE(1), NCT6683D_TACHO_READ_INDEX(1));
	printf("data = %x\n", data);

	for (i = 0; i < 32; i++) {
		printf("%x: %x, %x\n", i, \
				nct6683d_read_byte(NCT6683D_PAGE(1), 0xA0 + i), \
				nct6683d_read_word(NCT6683D_PAGE(1), NCT6683D_TEMP_VOL_READ_INDEX(i)));
	}

	sio_exit();

	return 0;
}
