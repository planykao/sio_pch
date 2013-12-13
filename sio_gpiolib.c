#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <sio_gpiolib.h>

void sio_enter(void)
{
	outb_p(0x87, EFER);
	outb_p(0x87, EFER);
}

void sio_exit(void)
{
	outb_p(0xAA, EFER);
}

unsigned char sio_read(int reg)
{
	unsigned char b;
	
	outb_p(reg, EFER); /* Sent index to EFER */
	b = inb_p(EFDR); /* Get the value from EFDR */
	return b; /* Return the value */
}

void sio_write(int reg, unsigned char val)
{
	outb_p(reg, EFER); /* Sent index at EFER */
	outb_p(val, EFDR); /* Send val_w at FEDR */
}

/* ldsel_reg: Logical Device Select 
 * ldnum:     Logical Device Number */
void sio_select(int ldnum)
{
	outb_p(SIO_LDSEL_REG, EFER);
	outb_p(ldnum, EFDR);
}

void sio_gpio_enable(int ldnum)
{
	unsigned char b;
	
	/*sio_write(7, 9);*/ /* Select Logical device number 9 */
	/* CR 30h of Logical Device Number 9 is enable register for GPIO1~7 */
	sio_select(ldnum);

	/* Active GPIO7 Group */
	b = sio_read(SIO_ENABLE_REG); /* Read the value of CR 30h of Logical device 9 */
	DBG("b = %x\n", b);
	b |= SIO_GPIO7_EN_OFFSET; /* Set bit7 to 1 to enable GPIO7 Group */
	sio_write(SIO_ENABLE_REG, b); /* Write the value at CR 30h of Logical device 9 */
}

unsigned char sio_gpio_get(int gpio) {
	return ((sio_read(SIO_GPIO7_DATA_REG) >> gpio) & 0x1);
}

void sio_gpio_set(int gpio, int value)
{
	unsigned char b;

	b = sio_read(SIO_GPIO7_DATA_REG);
	b &= ~(0x1 << gpio); /* clear */
	b |= (value << gpio);
	sio_write(SIO_GPIO7_DATA_REG, b);
}

void sio_gpio_dir_in(int gpio)
{
	unsigned char b;

	b = sio_read(SIO_GPIO7_DIR_REG);
	b |= (0x1 << gpio); /* set 1 for input */
	sio_write(SIO_GPIO7_DIR_REG, b);
}

void sio_gpio_dir_out(int gpio, int value)
{
	unsigned char b;

	/* direction */
	b = sio_read(SIO_GPIO7_DIR_REG);
	b &= ~(0x1 << gpio); /* clear to 0 for output */
	sio_write(SIO_GPIO7_DIR_REG, b);

	/* data */
	sio_gpio_set(gpio, value);
}
