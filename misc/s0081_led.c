#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#include <sitest.h>
#include <libsio.h>
#include <libpch.h>

#define LOW_GPIO_BASE_ADDR  0x0000 /* Refer to AST Datasheet page 416 */
#define HIGH_GPIO_BASE_ADDR 0x1E78		

#define AST_GPIO_NUM  19

struct gpio_groups
{
	char name;
	unsigned long int data_offset;
	unsigned long int dir_offset;
};

int ast_gpio_init(struct gpio_groups *gg, int value);
void usage(void);

int main(int argc, char *argv[])
{
	int value;
	unsigned int data;
	
	struct gpio_groups gg[AST_GPIO_NUM] = {
		{'A', 0x00, 0x04}, {'B', 0x01, 0x05}, \
        {'C', 0x02, 0x06}, {'D', 0x03, 0x07}, \
		{'E', 0x20, 0x24}, {'F', 0x21, 0x25}, \
        {'G', 0x22, 0x26}, {'H', 0x23, 0x27}, \
		{'I', 0x70, 0x74}, {'J', 0x71, 0x75}, \
        {'K', 0x72, 0x76}, {'L', 0x73, 0x77}, \
		{'M', 0x78, 0x7C}, {'N', 0x79, 0x7D}, \
        {'O', 0x7A, 0x7E}, {'P', 0x7B, 0x7F}, \
		{'Q', 0x80, 0x84}, {'R', 0x81, 0x85}, \
        {'S', 0x82, 0x86}
	};

	if (argc != 2)
		usage();

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	EFER = 0x2E;
	EFDR = 0x2F;

	value = atoi(argv[1]);
	if (value != 0 && value != 1)
		usage();

	sio_enter("AST1300");

	/* change command source to LPC */
	data = sio_ilpc2ahb_read(0xE3, 0x1E78);
	data |= (0x1 << 0);
	sio_ilpc2ahb_write(data, 0xE3, 0x1E78);
	data = sio_ilpc2ahb_read(0x91, 0x1E78);
	data |= (0x1 << 0);
	sio_ilpc2ahb_write(data, 0x91, 0x1E78);

	ast_gpio_init(gg, value);
	sio_exit();

	return 0;
}

/*
 * ast_gpio_init(), initial GPIOs and set the direction to output.
 */
int ast_gpio_init(struct gpio_groups *gg, int value)
{
	unsigned int data;
	int i, ret, offset;

	/* GPIOP0 ~ 7, change direction to output */
	data = sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[12].dir_offset, \
                                 HIGH_GPIO_BASE_ADDR);
	DBG("0x%02X dir: 0x%08X, ", gg[12].dir_offset, data);

	data |= (0xFF << 24);
	sio_ilpc2ahb_writel(data, LOW_GPIO_BASE_ADDR + gg[12].dir_offset, \
                       HIGH_GPIO_BASE_ADDR);

	/* set output data */
	data = sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[12].data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	DBG("0x%02X data: 0x%08X\n", gg[12].data_offset, data);

	if (value)
		data &= ~(0xFF << 24);
	else /* pull down to light led */
		data |= (0xFF << 24);
	sio_ilpc2ahb_writel(data, LOW_GPIO_BASE_ADDR + gg[12].data_offset, \
                       HIGH_GPIO_BASE_ADDR);

	DBG("0x%02X dir: 0x%08X, ", gg[12].dir_offset, \
			sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[12].dir_offset, \
			HIGH_GPIO_BASE_ADDR));
	DBG("0x%02X data: 0x%08X\n", gg[12].data_offset, \
			sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[12].data_offset, \
			HIGH_GPIO_BASE_ADDR));


	/* GPIOJ0 ~ 3, change direction to output */
	data = sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[8].dir_offset, \
                             HIGH_GPIO_BASE_ADDR);
	DBG("0x%02X dir: 0x%08X, ", gg[8].dir_offset, data);

	data |= (0x0F << 8); /* output */
	sio_ilpc2ahb_writel(data, LOW_GPIO_BASE_ADDR + gg[8].dir_offset, \
                       HIGH_GPIO_BASE_ADDR);

	/* set output data */
	data = sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[8].data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	DBG("0x%02X data: 0x%08X\n", gg[8].data_offset, data);

	if (value)
		data &= ~(0x0F << 8);
	else
		data |= (0x0F << 8); /* pull down to light led */

	sio_ilpc2ahb_writel(data, LOW_GPIO_BASE_ADDR + gg[8].data_offset, \
                       HIGH_GPIO_BASE_ADDR);

	DBG("0x%02X dir: 0x%08X, ", gg[8].dir_offset, \
			sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[8].dir_offset, \
			HIGH_GPIO_BASE_ADDR));
	DBG("0x%02X data: 0x%08X\n", gg[8].data_offset, \
			sio_ilpc2ahb_readl(LOW_GPIO_BASE_ADDR + gg[8].data_offset, \
			HIGH_GPIO_BASE_ADDR));
}

void usage(void)
{
	printf("./s0081_led value, value = 1(on) or 0(off)\n");
	exit(1);
}

