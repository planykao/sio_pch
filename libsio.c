#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <libsio.h>
#include <sitest.h>

/* Enter the Extended Function Mode */
void sio_enter(char *chip)
{
	if (strncmp("AST1300", chip, 7) == 0) {
		outb_p(0xA5, EFER);
		outb_p(0xA5, EFER);
		DBG("chip = %s, EFER = %x, EFDR = %x\n", chip, EFER, EFDR);
	} else {
		outb_p(0x87, EFER);
		outb_p(0x87, EFER);
		DBG("chip = %s, EFER = %x, EFDR = %x\n", chip, EFER, EFDR);
	}
}

/* Exit the Extended Function Mode */
void sio_exit(void)
{
	outb_p(0xAA, EFER);
}

int sio_read(int reg)
{
	int b;
	
	outb_p(reg, EFER); /* Sent index to EFER */
	b = inb_p(EFDR); /* Get the value from EFDR */
	return b; /* Return the value */
}

void sio_write(int reg, int val)
{
	outb_p(reg, EFER); /* Sent index at EFER */
	outb_p(val, EFDR); /* Send val_w at FEDR */
}

int sio_read_reg(int index, int address)
{
	outb_p(index, address);
	return inb_p(address + 1);
}

void sio_write_reg(int index, int address)
{
	outb_p(index, address);
}

/* ldsel_reg: Logical Device Select 
 * ldnum:     Logical Device Number */
void sio_select(int ldnum)
{
	DBG("SIO_LDSEL_REG = %x, ldnum = %x\n", SIO_LDSEL_REG, ldnum);
	outb_p(SIO_LDSEL_REG, EFER);
	outb_p(ldnum, EFDR);
}

void sio_gpio_enable(int ldnum)
{
	int b;
	
	/* CR 30h of Logical Device Number 9 is enable register for GPIO1~7 */
	sio_select(ldnum);

	/* Active GPIO7 Group */
	/* Read the value of CR 30h of Logical device 9 */
	b = sio_read(SIO_ENABLE_REG);
	DBG("b = %x\n", b);
	b |= SIO_GPIO7_EN_OFFSET; /* Set bit7 to 1 to enable GPIO7 Group */
	/* Write the value at CR 30h of Logical device 9 */
	sio_write(SIO_ENABLE_REG, b); 
}

void sio_logical_device_enable(int bit)
{
	int b;
	
	/* Read the value from CR 30h of Logical Device register */
	b = sio_read(SIO_ENABLE_REG);
	DBG("read from CR30h = %x\n", b);
	b |= (0x1 << bit); /* Set bitN to 1 to enable Logical Device */
	DBG("write %x to CR30h\n", b);
	/* Write the value to CR 30h of Logical device */
	sio_write(SIO_ENABLE_REG, b);
}

int sio_gpio_get(int gpio) {
	return ((sio_read(SIO_GPIO7_DATA_REG) >> gpio) & 0x1);
}

void sio_gpio_set(int gpio, int value)
{
	int b;

	b = sio_read(SIO_GPIO7_DATA_REG);
	b &= ~(0x1 << gpio); /* clear */
	b |= (value << gpio);
	sio_write(SIO_GPIO7_DATA_REG, b);
}

void sio_gpio_dir_in(int gpio)
{
	int b;

	b = sio_read(SIO_GPIO7_DIR_REG);
	b |= (0x1 << gpio); /* set 1 for input */
	sio_write(SIO_GPIO7_DIR_REG, b);
}

void sio_gpio_dir_out(int gpio, int value)
{
	int b;

	/* direction */
	b = sio_read(SIO_GPIO7_DIR_REG);
	b &= ~(0x1 << gpio); /* clear to 0 for output */
	sio_write(SIO_GPIO7_DIR_REG, b);

	/* data */
	sio_gpio_set(gpio, value);
}

/*
 * For AST1300
 */
unsigned int sio_ilpc2ahb_read(int lr, int hr)
{
	unsigned int b;
	unsigned int mod;

	mod = lr % 4; /* Address must be multiple of 4 */
	lr = lr - mod;

	/* 
	 * Setup address
	 * 0xF0: SIO iLPC2AHB address bit[31:24]
	 * 0xF1: SIO iLPC2AHB address bit[23:16]
	 * 0xF2: SIO iLPC2AHB address bit[15:8]   
	 * 0xF3: SIO iLPC2AHB address bit[7:0]
	 */
	sio_write(0xF0, hr >> 8);
	sio_write(0xF1, hr & 0xFF);
	sio_write(0xF2, lr >> 8);
	sio_write(0xF3, lr & 0xFF);

	/* Read to trigger SIO iLPC2AHB write command */
	sio_read(0xFE);

	/*
	 * Read data
	 * 0xF4: SIO iLPC2AHB data bit[31:24]
	 * 0xF5: SIO iLPC2AHB data bit[23:16]
	 * 0xF6: SIO iLPC2AHB data bit[15:8]
	 * 0xF7: SIO iLPC2AHB data bit[7:0]
	 */
	b = sio_read(0xF7 - mod); /* Get the value */

	DBG("b = %x\n", b);
	return b;
}

void sio_ilpc2ahb_write(unsigned char val_w, unsigned int lw, unsigned int hw)
{
	/* 
	 * Setup address
	 * 0xF0: SIO iLPC2AHB address bit[31:24]
	 * 0xF1: SIO iLPC2AHB address bit[23:16]
	 * 0xF2: SIO iLPC2AHB address bit[15:8]
	 * 0xF3: SIO iLPC2AHB address bit[7:0]
	 */
	sio_write(0xF0, hw >> 8);
	sio_write(0xF1, hw & 0xFF);
	sio_write(0xF2, lw >> 8);
	sio_write(0xF3, lw & 0xFF);

	/*
	 * Write data
	 * 0xF4: SIO iLPC2AHB data bit[31:24]
	 * 0xF5: SIO iLPC2AHB data bit[23:16]
	 * 0xF6: SIO iLPC2AHB data bit[15:8]
	 * 0xF7: SIO iLPC2AHB data bit[7:0]
	 */
	sio_write(0xF7, val_w);

	/* Write 0xCF to trigger SIO iLPC2AHB write command */
	sio_write(0xFE, 0xCF);
}

void sio_ilpc2ahb_setup(void)
{
	unsigned int b;

	/* select logical device iLPC2AHB */
	sio_select(SIO_LPC2AHB_LDN);
	/* enable iLPC2AHB */
	sio_logical_device_enable(SIO_LPC2AHB_EN);
	/* Set Length to 1 Byte */
	b = sio_read(0xF8);
	b &= ~(0x03);
	sio_write(0xF8, b);
}
/* For AST1300 end */

