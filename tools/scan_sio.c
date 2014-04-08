/*
 * sio_scan.c, sacn and identify the SuperIO.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <libsio.h>

#define SIO_UNLOCK_KEY      0x87
#define SIO_LOCK_KEY        0xAA

#define SIO_DEVICE_ID_REG   0x20
#define SIO_VENDOR_ID_REG   0x23

#define SIO_NUVOTON_ID      0x5CA3 /* Manufacturers ID, same as Winbond */
#define SIO_NCT6106_ID      0xC450 /* Chipset ID */
#define SIO_NCT6775_ID      0xB470 /* Chipset ID */
#define SIO_NCT6776_ID      0xC330 /* Chipset ID */
#define SIO_NCT6779_ID      0xC560 /* Chipset ID */
#define SIO_NCT6791_ID      0xC800 /* Chipset ID */
#define SIO_NCT_ID_MASK     0xFFF0

#define SIO_FINTEK_ID       0x1934 /* Manufacturers ID */
#define SIO_F71808E_ID      0x0901 /* Chipset ID */
#define SIO_F71808A_ID      0x1001 /* Chipset ID */
#define SIO_F71858_ID       0x0507 /* Chipset ID */
#define SIO_F71862_ID       0x0601 /* Chipset ID */
#define SIO_F71868AD_ID     0x1106 /* Chipset ID */
#define SIO_F71869_ID       0x0814 /* Chipset ID */
#define SIO_F71869A_ID      0x1007 /* Chipset ID */
#define SIO_F71882_ID       0x0541 /* Chipset ID */
#define SIO_F71889_ID       0x0723 /* Chipset ID */
#define SIO_F71889E_ID      0x0909 /* Chipset ID */
#define SIO_F71889A_ID      0x1005 /* Chipset ID */
#define SIO_F8000_ID        0x0581 /* Chipset ID */
#define SIO_F81865_ID       0x0704 /* Chipset ID */

#define VID_ERROR           -1
#define DEVID_ERROR         -2
#define NOT_SUPPORT         "NOT_SUPPORT"

static int sio_readw(int, int);
static int sio_find(int);

#if 0
enum nuvoton
{
	nct6776,
	nct6779
};

enum fintek
{
	f71868a,
	f71869,
	f71869a,
	f71889a,
	f71889e
};
#endif

char devid[15], vid[15];

int sio_readw(int index, int addr)
{
	int data;

	data = sio_read_reg(index, addr) << 8;
	data |= sio_read_reg(index + 1, addr);

	return data;
}

int sio_find(int addr)
{
	int id, ids;

	/* sio enter */
	outb_p(SIO_UNLOCK_KEY, addr);
	outb_p(SIO_UNLOCK_KEY, addr);

	/* read vendor id */
	id = sio_readw(SIO_VENDOR_ID_REG, addr);

	if (id == SIO_FINTEK_ID) {
		strcpy(vid, "Fintek");

		/* read device id */
		id = sio_readw(SIO_DEVICE_ID_REG, addr);

		switch(id) {
			case SIO_F71808E_ID:
				strcpy(devid, "F71808E");
				break;
			case SIO_F71808A_ID:
				strcpy(devid, "F71808A");
				break;
			case SIO_F71858_ID:
				strcpy(devid, "F71858");
				break;
			case SIO_F71862_ID:
				strcpy(devid, "F71862");
				break;
			case SIO_F71868AD_ID:
				strcpy(devid, "F71868AD");
				break;
			case SIO_F71869_ID:
				strcpy(devid, "F71869");
				break;
			case SIO_F71869A_ID:
				strcpy(devid, "F71869A");
				break;
			case SIO_F71882_ID:
				strcpy(devid, "F71882");
				break;
			case SIO_F71889_ID:
				strcpy(devid, "F71889");
				break;
			case SIO_F71889E_ID:
				strcpy(devid, "F71889E");
				break;
			case SIO_F71889A_ID:
				strcpy(devid, "F71889A");
				break;
			case SIO_F8000_ID:
				strcpy(devid, "F8000");
				break;
			case SIO_F81865_ID:
				strcpy(devid, "F81865");
				break;
			case 0xFF:
				id = DEVID_ERROR;
				break;
			default:
				strcpy(devid, NOT_SUPPORT);
				break;
		}
	} else if (id != 0xFFFF) { /* Nuvoton doesn't put VerdorID at 0x23 */
		/* read device id */
		id = sio_readw(SIO_DEVICE_ID_REG, addr);

		if (id != 0xFFFF)
			strcpy(vid, "Nuvoton");

		/* The last bit is version, 1: A version, 2: B version, 3: C version */
		ids = id & SIO_NCT_ID_MASK;

		switch (ids) {
			case SIO_NCT6106_ID:
				strcpy(devid, "NCT6106");
				break;
			case SIO_NCT6775_ID:
				strcpy(devid, "NCT6775");
				break;
			case SIO_NCT6776_ID:
				strcpy(devid, "NCT6776");
				break;
			case SIO_NCT6779_ID:
				strcpy(devid, "NCT6779");
				break;
			case SIO_NCT6791_ID:
				strcpy(devid, "NCT6791");
				break;
			case 0xFFFF:
				id = DEVID_ERROR;
				break;
			default:
				strcpy(devid, NOT_SUPPORT);
				break;
		}
	} else
		id = VID_ERROR;

	/* sio exit */
	outb_p(SIO_LOCK_KEY, addr);

	return id;
}

int main(void)
{
	int ret = 0, i;

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	ret = sio_find(0x4E);

	if (ret == VID_ERROR) {
		/* scan on 0x2E */
		ret = sio_find(0x2E);
		if (ret == VID_ERROR) {
			printf("can't find superio\n");
			goto exit;
		} else if (ret == DEVID_ERROR) {
			printf("Read SuperIO device error\n");
			goto exit;
		}
	} else if (ret == DEVID_ERROR) {
		printf("Read SuperIO device error\n");
		goto exit;
	}

	printf("Vendor: %s\nChip(ID): %s(0x%X)\n", vid, devid, ret);

exit:
	return 0;
}
