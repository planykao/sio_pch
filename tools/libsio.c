#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/io.h>
#include <libsio.h>
#include <sitest.h>

/* Enter the Extended Function Mode */
void sio_enter(char *chip)
{
	if (strncmp("AST", chip, 3) == 0) {
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

/* Read value from register of SuperIO */
int sio_read_reg(int index, int address)
{
	outb_p(index, address);
	return inb_p(address + 1);
}

/* Write value to register of SuperIO */
void sio_write_reg(int index, int address)
{
	outb_p(index, address);
}

/* 
 * ldsel_reg: Logical Device Select 
 * ldnum:     Logical Device Number
 */
void sio_select(int ldnum)
{
	DBG("SIO_LDSEL_REG = %x, ldnum = %x\n", SIO_LDSEL_REG, ldnum);
	outb_p(SIO_LDSEL_REG, EFER);
	outb_p(ldnum, EFDR);
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

/* 
 * sio_gpio_enable(), select Logical Device Number and enable for input GPIO.
 * ldnum, logical device number
 * offset, enable bit offset
 */
void sio_gpio_enable(int ldnum, int offset)
{
	int b;
	
	/* CR 30h of Logical Device Number 9 is enable register for GPIO1~7 */
	sio_select(ldnum);

	/* Active GPIO7 Group */
	/* Read the value of CR 30h of Logical device 9 */
	b = sio_read(SIO_ENABLE_REG);
	DBG("b = %x\n", b);
	b |= 0x1 << offset; /* Set bit7 to 1 to enable GPIO7 Group */
	/* Write the value at CR 30h of Logical device 9 */
	sio_write(SIO_ENABLE_REG, b); 
}

/*
 * sio_gpio_get(), get data from GPIO.
 * index is direction register, data register = direction index + 1
 */
int sio_gpio_get(int gpio, int index)
{
	return ((sio_read(index + 1) >> (gpio % 10)) & 0x1);
}

/*
 * sio_gpio_set(), set data to GPIO.
 * index is direction register, data register = direction index + 1
 */
void sio_gpio_set(int gpio, int value, int index)
{
	int b;

	b = sio_read(index + 1); /* data index = dir index + 1*/
	b &= ~(0x1 << (gpio % 10)); /* clear */
	b |= (value << (gpio % 10));
	sio_write(index + 1, b);
}

/*
 * sio_gpio_dir_in(), set gpio to input.
 * Nuvoton: 0 is output, 1 is input.
 * Fintek:  1 is output, 0 is input.
 *
 * io, input, use the following to make it more readable:
 *
 * NCT_GPIO_IN    1
 * FIN_GPIO_IN    0
 *
 * i.e., sio_gpio_dir_in(gpio, index, NCT_GPIO_IN)
 *       sio_gpio_dir_in(gpio, index, FIN_GPIO_IN)
 */
void sio_gpio_dir_in(int gpio, int index, int io)
{
	int b;

	b = sio_read(index);
	b &= ~(0x1 << (gpio % 10)); /* set 1 for input */
	b |= (io << (gpio % 10)); /* set 1 for input */
	sio_write(index, b);
}

/*
 * sio_gpio_dir_out(), set gpio to output and pull HIGH or LOW
 * Nuvoton: 0 is output, 1 is input.
 * Fintek:  1 is output, 0 is input.
 *
 * io, output, use the following to make it more readable:
 *
 * NCT_GPIO_OUT    0
 * FIN_GPIO_OUT    1
 *
 * i.e., sio_gpio_dir_out(gpio, value, index, NCT_GPIO_OUT)
 *       sio_gpio_dir_out(gpio, value, index, FIN_GPIO_OUT)
 */
void sio_gpio_dir_out(int gpio, int value, int index, int io)
{
	int b;

	/* direction */
	b = sio_read(index);
	b &= ~(0x1 << (gpio % 10)); /* clear to 0 for output */
	b |= (io << (gpio % 10));
	sio_write(index, b);

	/* data */
	sio_gpio_set(gpio, value, index);
}

/*
 * For AST1300
 */
unsigned int sio_ilpc2ahb_read(int lr, int hr)
{
	unsigned int b;
	unsigned int mod;

	sio_ilpc2ahb_setup(0); /* Setup iLPC2AHB and set data length to 1 bytes */
	
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

	return b;
}

unsigned int sio_ilpc2ahb_readl(int lr, int hr)
{
	unsigned int b;
	unsigned int mod;

	sio_ilpc2ahb_setup(2); /* Setup iLPC2AHB and set data length to 1 bytes */
	
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
	b = sio_read(0xF7); /* Get the value */
	b |= sio_read(0xF6) << 8; /* Get the value */
	b |= sio_read(0xF5) << 16; /* Get the value */
	b |= sio_read(0xF4) << 24; /* Get the value */

	return b;
}

void sio_ilpc2ahb_write(unsigned char val_w, unsigned int lw, unsigned int hw)
{
	
	sio_ilpc2ahb_setup(0); /* Setup iLPC2AHB and set data length to 1 bytes */
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

void sio_ilpc2ahb_writel(unsigned int val_w, unsigned int lw, unsigned int hw)
{
	sio_ilpc2ahb_setup(2); /* Setup iLPC2AHB and set data length to 4 bytes */
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
	sio_write(0xF4, (val_w >> 24) & 0xFF);
	sio_write(0xF5, (val_w >> 16) & 0xFF);
	sio_write(0xF6, (val_w >> 8) & 0xFF);
	sio_write(0xF7, val_w & 0xFF);

	/* Write 0xCF to trigger SIO iLPC2AHB write command */
	sio_write(0xFE, 0xCF);
}

/*
 * sio_ilpc2ahb_setup(), setup the ilpc2ahb.
 * len = 0, 1 byte.
 * len = 1, 2 bytes.
 * len = 2, 4 bytes.
 */
void sio_ilpc2ahb_setup(int len)
{
	unsigned int b;

	/* select logical device iLPC2AHB */
	sio_select(SIO_LPC2AHB_LDN);
	/* enable iLPC2AHB */
	sio_logical_device_enable(SIO_LPC2AHB_EN);
	/* Set Length */
	b = sio_read(0xF8);
	b &= ~(0x03);
	b |= len;
	sio_write(0xF8, b);
}
/* For AST1300 end */

/* 
 * f71889ad_get_gpio_dir_index(), get gpio direction index for F71889AD.
 * gpio data index = direction index + 1
 */
int f71889ad_get_gpio_dir_index(int gpio)
{
	if (gpio >= 0 && gpio <= 6)
		return 0xF0;
	else if (gpio >= 10 && gpio <= 16)
		return 0xE0;
	else if (gpio >= 25 && gpio <= 27)
		return 0xD0;
	else if (gpio >= 30 && gpio <= 37)
		return 0xC0;
	else if (gpio >= 40 && gpio <= 47)
		return 0xB0;
	else if (gpio >= 50 && gpio <= 54)
		return 0xA0;
	else if (gpio >= 60 && gpio <= 67)
		return 0x90;
	else if (gpio >= 70 && gpio <= 77)
		return 0x80;
	else
		return -1;
}

/* 
 * nct_get_gpio_dir_index(), get gpio direction index for Nuvoton NCT6776F 
 * SuperIO. 
 * gpio data index = direction index + 1
 */
int nct_get_gpio_dir_index(int gpio)
{
	if (gpio >= 0 && gpio <= 5) /* GPIO0, Logical Device 8 */
		return 0xE0;
	else if (gpio >= 10 && gpio <= 17) /* GPIO1, Logical Device 8 */
		return 0xF0;
	else if (gpio >= 20 && gpio <= 27) /* GPIO2, Logical Device 9 */
		return 0xE0;
	else if (gpio >= 30 && gpio <= 37) /* GPIO3, Logical Device 9 */
		return 0xE4;
	else if (gpio >= 40 && gpio <= 47) /* GPIO4, Logical Device 9 */
		return 0xF0;
	else if (gpio >= 50 && gpio <= 54) /* GPIO5, Logical Device 9 */
		return 0xF4;
	else if (gpio >= 60 && gpio <= 67) /* GPIO6, Logical Device 7 */
		return 0xF4;
	else if (gpio >= 70 && gpio <= 77) /* GPIO7, Logical Device 7 */
		return 0xE0;
	else if (gpio >= 80 && gpio <= 87) /* GPIO8, Logical Device 7 */
		return 0xE4;
	else if (gpio >= 90 && gpio <= 97) /* GPIO9, Logical Device 7 */
		return 0xE8;
	else if (gpio == 100) /* GPIOA, Logical Device 8 */
		return 0xE0;
	else
		return -1;
}

/* 
 * nct5_get_gpio_dir_index(), get gpio direction index for Nuvoton NCT5XXX 
 * SuperIO.
 * gpio data index = direction index + 1
 */
int nct5xxx_get_gpio_dir_index(int gpio)
{
	if (gpio == 0 || gpio == 4) /* GPIO0, Logical Device 8 */
		return 0xE0;
	else if (gpio >= 20 && gpio <= 26) /* GPIO2, Logical Device 9 */
		return 0xE0;
	else if (gpio == 41 || gpio == 42) /* GPIO4, Logical Device 9 */
		return 0xF0;
	else if (gpio == 54 || gpio == 56 || gpio == 57) /* GPIO5, Logical Device 9 */
		return 0xF4;
	else if (gpio == 74 || gpio <= 75) /* GPIO7, Logical Device 7 */
		return 0xE0;
	else if (gpio >= 80 && gpio <= 87) /* GPIO8, Logical Device 7 */
		return 0xE4;
	else
		return -1;
}

/*
 * nct5xxx_multi_func_pin(), setup multi-function pin for Nuvoton NCT5XXXX,
 * this should be done by BIOS, not application.
 * Please refer to datasheet page 255 for more detail.
 */
void nct5xxx_multi_func_pin(int gpio)
{
	int b;

	if (gpio == 0) {
		b = sio_read(0x1C);
		DBG("0x1C: %x\n", b);
		b |= (0x1 << 0);
		DBG("0x1C: %x\n", b);
		sio_write(0x1C, b);

		sio_select(0x08);
		b = sio_read(0xE4);
		b &= ~(0x1 << 0);
		DBG("LDN: 0x08, offset: 0xE4, value: %x\n", b);
		sio_write(0xE4, b);
	} else if (gpio == 4) {
		b = sio_read(0x1C);
		DBG("0x1C: %x\n", b);
		b |= (0x1 << 5);
		DBG("0x1C: %x\n", b);
		sio_write(0x1C, b);
	} else if (gpio == 20 || gpio == 21) {
		b = sio_read(0x2A);
		DBG("0x2A: %x\n", b);
		b |= 0x1;
		DBG("0x2A: %x\n", b);
		sio_write(0x2A, b);

		sio_select(0x09);
		b = sio_read(0xE9);
		b &= ~(0x1 << (gpio % 10));
		DBG("LDN: 0x08, offset: 0xE4, value: %x\n", b);
		sio_write(0xE9, b);
	} else if (gpio == 22 || gpio == 23) {
		b = sio_read(0x2A);
		DBG("0x2A: %x\n", b);
		b |= 0x1 << 1;
		DBG("0x2A: %x\n", b);
		sio_write(0x2A, b);

		sio_select(0x09);
		b = sio_read(0xE9);
		b &= ~(0x1 << (gpio % 10));
		DBG("LDN: 0x08, offset: 0xE4, value: %x\n", b);
		sio_write(0xE9, b);
	} else if (gpio == 26) {
		b = sio_read(0x2C);
		DBG("0x2C: %x\n", b);
		b &= ~(0x1 << 0);
		DBG("0x2C: %x\n", b);
		sio_write(0x2C, b);
	} else if (gpio == 54 || gpio == 56 || gpio == 57){
		sio_select(0x09);
		b = sio_read(0xEB);
		b &= ~(0x1 << (gpio % 10));
		DBG("LDN: 0x09, offset: 0xEB, value: %x\n", b);
		sio_write(0xEB, b);
	} else if (gpio == 74 || gpio == 75) {
		b = sio_read(0x2B);
		DBG("0x2B: %x\n", b);
		b |= (0x1 << (gpio % 10 + 1));
		DBG("0x2B: %x\n", b);
		sio_write(0x2B, b);

		sio_select(0x07);
		b = sio_read(0xEC);
		b &= ~(0x1 << (gpio % 10));
		DBG("LDN: 0x08, offset: 0xE4, value: %x\n", b);
		sio_write(0xEC, b);
	} else if (gpio >=80 && gpio <=87) {
		sio_select(0x07);
		b = sio_read(0xED);
		b &= ~(0x1 << (gpio % 10));
		DBG("LDN: 0x08, offset: 0xE4, value: %x\n", b);
		sio_write(0xED, b);
	}
}

/*
 * sio_get_gpio_dir_index(), get gpio direction index for SupereIO
 */
int sio_get_gpio_dir_index(char *chip, int gpio)
{
	int ret = -1;

	if (strncmp(chip, "F71889AD", 8) == 0)
		ret = f71889ad_get_gpio_dir_index(gpio);
	else if (strncmp(chip, "NCT6", 4) == 0)
		ret = nct_get_gpio_dir_index(gpio);
	else if (strncmp(chip, "NCT5", 4) == 0)
		ret = nct5xxx_get_gpio_dir_index(gpio);

	return ret;
}




int sio_gpio_get_en_ldn(char *name, int gpio)
{
	int quotient, ldn = -1;

	quotient = gpio / 10;

	if (strncmp(name, "NCT6776", 7) == 0) {
		if (quotient == 0 || quotient == 10) /* FIXME */
			ldn = 8;
		else if (quotient >= 1 && quotient <= 7)
			ldn = 9;
		else if (quotient == 8 || quotient == 9)
			ldn = 7;
	} else if (strncmp(name, "NCT6779", 7) == 0) {
		if (quotient == 0)
			ldn = 8;
		else if (quotient >= 1 && quotient <= 8)
			ldn = 9;
	} else if (strncmp(name, "NCT5", 4) == 0) {
		if (quotient == 0)
			ldn = 8;
		else if ((quotient >= 2 && quotient <= 5) \
				|| quotient == 7 || quotient == 8)
			ldn = 9;
	}

	return ldn;
}

int sio_gpio_get_en_offset(char *name, int gpio)
{
	int quotient, offset = -1;

	quotient = gpio / 10;

	if (strncmp(name, "NCT6776", 7) == 0) {
		if (quotient == 0)
			offset = 1;
		else if (quotient >= 1 && quotient <= 7)
			offset = quotient;
		else if (quotient == 8 || quotient == 9)
			offset = quotient - 8;
		else if (quotient == 10) /* FIXME */
			offset = 2;
	} else if (strncmp(name, "NCT6779", 7) == 0) {
		if (quotient == 0)
			offset = 1;
		else if (quotient >= 1 && quotient <= 7)
			offset = quotient;
		else if (quotient == 8)
			offset = 0;
	} else if (strncmp(name, "NCT5", 4) == 0) {
		if (quotient == 0)
			offset = 1;
		else if ((quotient >= 2 && quotient <= 5) || quotient == 7)
			offset = quotient;
		else if (quotient == 8)
			offset = 0;
	}

	return offset;
}

int sio_gpio_get_ldn(char *name, int gpio)
{
	int quotient, ldn = -1;

	quotient = gpio / 10;

	if (strncmp(name, "NCT6776", 7) == 0) {
		if (quotient == 0 || quotient == 1)
			ldn = 8;
		else if (quotient >= 2 && quotient <= 5)
			ldn = 9;
		else if (quotient >= 6 && quotient <= 9)
			ldn = 7;
		else if (quotient == 10) /* FIXME */
			ldn = 17;
	} else if (strncmp(name, "NCT6779", 7) == 0) {
		if (quotient == 0 || quotient == 1)
			ldn = 8;
		else if (quotient >= 2 && quotient <= 5)
			ldn = 9;
		else if (quotient >= 6 && quotient <=8)
			ldn = 7;
	} else if (strncmp(name, "NCT5", 4) == 0) {
		if (quotient == 0)
			ldn = 8;
		else if (quotient >= 2 && quotient <= 5)
			ldn = 9;
		else if (quotient == 7 || quotient == 8)
			ldn = 7;
	}

	return ldn;
}
