#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <libpch.h>
#include <sitest.h>

/* re-assign the gpio number */
int gpio_setup_addr(unsigned long int *gpio_use_sel_addr, \
                            unsigned long int *gp_io_sel_addr, \
                            unsigned long int *gp_lvl_addr, \
							int gpio, unsigned long int base_addr)
{
	int new_gpio;

	if (gpio >= 0 && gpio <= 31) {
		*gpio_use_sel_addr = GPIO_USE_SEL1_ADDR(base_addr);
		*gp_io_sel_addr = GP_IO_SEL1_ADDR(base_addr);
		*gp_lvl_addr = GP_LVL1_ADDR(base_addr);
		new_gpio = gpio;
	} else if (gpio >= 32 && gpio <= 63) {
		*gpio_use_sel_addr = GPIO_USE_SEL2_ADDR(base_addr);
		*gp_io_sel_addr = GP_IO_SEL2_ADDR(base_addr);
		*gp_lvl_addr = GP_LVL2_ADDR(base_addr);
		new_gpio = gpio - 32;
	} else {
		*gpio_use_sel_addr = GPIO_USE_SEL3_ADDR(base_addr);
		*gp_io_sel_addr = GP_IO_SEL3_ADDR(base_addr);
		*gp_lvl_addr = GP_LVL3_ADDR(base_addr);
		new_gpio = gpio - 64;
	}

	return new_gpio;
}

/* Set GPIO to use as a GPIO, no native function */
void gpio_enable(unsigned long int gpio_use_sel_addr, int gpio)
{
	unsigned long int buf;
	
	buf = inl_p(gpio_use_sel_addr); /* Read the original value */
	buf |= (0x1 << gpio); /* Set the bit to 1 for GPIO mode */
	outl_p(buf, gpio_use_sel_addr); /* Output the value */
}

unsigned long int gpio_get(unsigned long int gp_lvl_addr, int gpio)
{
	unsigned long int buf;

	buf = inl_p(gp_lvl_addr);
	buf = (buf >> gpio);
	buf &= 0x1;
	
	return buf;
}

void gpio_set(unsigned long int gp_lvl_addr, int gpio, int value)
{
	unsigned long int buf;

	buf = inl_p(gp_lvl_addr); /* Read the original value */
	buf &= ~(0x1 << gpio); /* Set the bit to 0 for low level */
	buf |= (value << gpio); /* Set the bit to 1 for high level */
	outl_p(buf, gp_lvl_addr); /* Output the value */
	DBG("Set GPIO Level to %d\n", value);
}

void gpio_dir_in(unsigned long int gp_io_sel_addr, int gpio)
{
	unsigned long int buf;

	buf = inl_p(gp_io_sel_addr); /* Read the original value */
	DBG("buf = %x, gpio = %d\n", buf, gpio);
	buf |= (0x1 << gpio); /* Set the bit to 1 for input */
	DBG("buf = %x, gpio = %d\n", buf, gpio);
	outl_p(buf, gp_io_sel_addr); /* Output the value */
}

void gpio_dir_out(unsigned long int gp_io_sel_addr, \
                         unsigned long int gp_lvl_addr, int gpio, int value)
{
	unsigned long int buf;
	
	buf = inl_p(gp_io_sel_addr); /* Read the original value */
	DBG("buf = %x, gpio = %d\n", buf, gpio);
	buf &= ~(0x1 << gpio); /* Set the bit to 0 for output */
	DBG("buf = %x, gpio = %d\n", buf, gpio);
	outl_p(buf, gp_io_sel_addr); /* Output the value */

	buf = inl_p(gp_lvl_addr); /* Read the original value */
	buf &= ~(0x1 << gpio); /* Set the bit to 0 for low level */
	buf |= (value << gpio); /* Set the bit to 1 for high level */
	outl_p(buf, gp_lvl_addr); /* Output the value */
	DBG("Set GPIO[%d] Level to %s\n", gpio, value ? "HIGH" : "LOW");
}

void gpio_blink(unsigned long int base, int gpio, int value)
{
	unsigned long int buf;

	buf = inl_p(base + GPO_BLINK);
	buf &= ~(0x1 << gpio);
	buf |= (value << gpio);
	outl_p(buf, base + GPO_BLINK);
	DBG("Set GPIO[%d] blink to %d\n", gpio, value);
}

