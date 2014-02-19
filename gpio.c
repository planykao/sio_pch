/*
 * Set GPIOs direction to output and pull high or low.
 *
 * Usage: 
 *     # ./gpio -c CONFIG -g GPIOs -o HIGH/LOW
 *
 * For example, set GPIO70 to HIGH on S0361: 
 *     # ./gpio -c S0361.conf -g 70 -o high
 *
 * Set GPIO72 to LOW on S0361:
 *     # ./gpio -c S0361.conf -g 72 -o low
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
static void sio_gpio_config(int, int);

int total;

int main(int argc, char *argv[])
{
	char args, *filename, chip[10];
	int i = 0, gpio[10] = {-1}, level = -1;
	unsigned int gpio_base_addr;

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	/* parsing arguments */
	while ((args = getopt(argc, argv, "c:g:ho:")) != -1) {
		switch (args) {
			case 'c':
				filename = optarg;
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
				break;
			case 'h':
			case '?':
				usage(1);
				break;
		}
	}

#ifdef DEBUG
	DBG("filename = %s\n", filename);
	for (i = 0; i < 4; i++)
		DBG("gpio[%d] = %d\n", i, gpio[i]);
	DBG("level = %d\n", level);
#endif

	if (filename == NULL || gpio[0] == -1 || level == -1)
		usage(1);

	/* read configuration file */
	read_config(filename, chip, &gpio_base_addr);
	DBG("chip = %s, addr = %x\n", chip, gpio_base_addr);

	/* test start */
	if (strncmp(chip, "PCH", 3) == 0) {
		for (i = 0; i < total; i++) {
			gpio_config(gpio[i], level, gpio_base_addr);
		}
	} else {
		for (i = 0; i < total; i++) {
			EFER = gpio_base_addr;
			EFDR = EFER + 1;
			sio_gpio_config(gpio[i], level);
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

static void sio_gpio_config(int gpio, int level)
{
	int new_gpio;

	/* Enable GPIO7 group */
	sio_gpio_enable(SIO_GPIO_EN_REG, GPIO7);
	/* Select GPIO7 */
	sio_select(SIO_GPIO7_LDN);

	new_gpio = sio_gpio_calculate(gpio);
	if (new_gpio == -1) {
		ERR("GPIO number incorrect.\n");
		exit(-1);
	}

	sio_gpio_dir_out(new_gpio, level);

	printf("Set GPIO[%d] Level to %s\n", gpio, level ? "HIGH" : "LOW");
}

