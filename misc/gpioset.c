/*
 * Set GPIOs direction to output and pull high or low.
 *
 * Usage: 
 *     # ./gpio -a ADDR -g GPIOs -o 1/0
 *
 * For example, set PCH GPIO70 to HIGH: 
 *     # ./gpio -a 0x1C00 -g 70 -o 1
 *
 * Set GPIO72 to LOW on S0361:
 *     # ./gpio -a 0x2E -g 72 -o 0
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <unistd.h>
#include <string.h>
#include <sitest.h>
#include <libpch.h>
#include <libsio.h>

void usage(int);
int read_config(char *, char *, unsigned int *);
static void gpio_config(int, int, unsigned int);
static void sio_gpio_config(int, int, char *);

int total;

int main(int argc, char *argv[])
{
	char args, *filename, chip[10];
	int i = 0, gpio[10] = {-1}, level = -1;
	unsigned int gpio_base_addr = 0xFFFF;

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	/* parsing arguments */
	while ((args = getopt(argc, argv, "a:g:ho:")) != -1) {
		switch (args) {
			case 'a':
				gpio_base_addr = (unsigned int)strtol(optarg, NULL, 16);
				break;
			case 'g':
				optind--;
				for (; optind < argc && *argv[optind] != '-'; optind++)
					gpio[i++] = atoi(argv[optind]);
				total = i;
				break;
			case 'o':
				level = atoi(optarg);
				break;
			case ':':
			case 'h':
			case '?':
				usage(1);
				break;
		}
	}

	/* check the gpio base address */
	if (gpio_base_addr == 0x1C00 || gpio_base_addr == 0x500) { /* PCH */
		for (i = 0; i < total; i++) {
			gpio_config(gpio[i], level, gpio_base_addr);
		}
	} else { /* SIO */
		for (i = 0; i < total; i++) {
			EFER = gpio_base_addr;
			EFDR = EFER + 1;
			sio_gpio_config(gpio[i], level, chip);
		}
	}
	/* test end */

	return 0;
}

void usage(int i)
{
	if (i) {
		printf("Usage: ./gpio [-c config -g GPIOs -o num]\n");
		printf("Usage: ./gpio [-h]\n\n");
		printf("  -c, <config_file>  configuration file for target board\n");
		printf("  -g, #              GPIO number\n");
		printf("  -o, #              output level, 1 is HIGH, 0 is LOW\n");
		printf("  -h,                print this message and quit\n\n");
	} else {
		printf("Usage: [-c config|-g GPIOs|-h|-o]\n");
		printf("Try './gpio -h for more information\n'");
	}

	exit(1);
}

int read_config(char *filename, char *chip, unsigned int *address)
{
	FILE *fp;
	int i;

	fp = fopen(filename, "r");
	
	if (fp == NULL) {
		ERR("Fail to open <%s>, please enter the correct file name.\n", filename);
		return -1;
	}

	/* skip header */
	for (i = 0; i < 4; i++)
		fscanf(fp, "%*[^\n]\n", NULL);

	/* read chip and GPIO base address */
	fscanf(fp, "%s %x\n", chip, address);
	fclose(fp);

	return 0;
}

/* Set gpio direction to output and pull HIGH or LOW  */
static void gpio_config(int gpio, int level, unsigned int gpio_base_addr)
{
	unsigned long int gpio_use_sel_addr, gp_io_sel_addr, gp_lvl_addr;
	int new_gpio;

	/* Set gpio_out direction to output and pull low or high. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio, gpio_base_addr);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_out(gp_io_sel_addr, gp_lvl_addr, new_gpio, level);

	DBG("gpio_sel_addr = %x, gp_lvl_addr =%x\n", gp_io_sel_addr, gp_lvl_addr);
	printf("Set GPIO[%d] Level to %s\n", gpio, level ? "HIGH" : "LOW");
}

static void sio_gpio_config(int gpio, int level, char *chip)
{
	int index;

	/* Enable GPIO7 group */
	sio_gpio_enable(NCT_GPIO7_EN_LDN, GPIO7);
	/* Select GPIO7 */
	sio_select(NCT_GPIO7_LDN);
	index = sio_get_gpio_dir_index(chip, gpio);
	sio_gpio_dir_out(gpio, level, index, NCT_GPIO_OUT);

	printf("Set GPIO[%d] Level to %s\n", gpio, level ? "HIGH" : "LOW");
}

