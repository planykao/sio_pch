/*
 * Set GPIOs direction to output and pull high or low.
 *
 * Usage: 
 *     # ./gpio -c CONFIG -g GPIOs -o 1/0
 *
 * For example, set GPIO70 to HIGH on S0361: 
 *     # ./gpio -c S0361.conf -g 70 -o 1
 *
 * Set GPIO72 to LOW on S0361:
 *     # ./gpio -c S0361.conf -g 72 -o 0
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <unistd.h>
#include <string.h>
#include <sitest.h>
#include <libpch.h>
#include <libsio.h>

struct chip
{
	char name[10];
	int kind;
};

void usage(int);
int read_config(char *, char *, unsigned int *);
static void pch_gpio_config(int, int, unsigned int);
static int sio_gpio_config(int, int, struct chip *);
static int nct_get_kind(struct chip *);

enum nct_kinds {NCT6776, NCT6779D, NCT5533D};

int total;

int main(int argc, char *argv[])
{
	struct chip chip;
	char args, *filename;
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
			case 'h':
			case '?':
				usage(1);
				break;
		}
	}

#ifdef DEBUG
	DBG("filename = %s\n", filename);
	for (i = 0; i < total; i++)
		DBG("gpio[%d] = %d\n", i, gpio[i]);
	DBG("level = %d\n", level);
#endif

	if (filename == NULL || gpio[0] == -1 || level == -1)
		usage(1);

	/* read configuration file */
	read_config(filename, chip.name, &gpio_base_addr);
	DBG("chip.name = %s, addr = %x\n", chip.name, gpio_base_addr);

	/* test start */
	if (strncmp(chip.name, "PCH", 3) == 0) {
		for (i = 0; i < total; i++) {
			pch_gpio_config(gpio[i], level, gpio_base_addr);
		}
	} else {
		for (i = 0; i < total; i++) {
			EFER = gpio_base_addr;
			EFDR = EFER + 1;
			sio_enter(chip.name);
			sio_gpio_config(gpio[i], level, &chip);
			sio_exit();
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
static void pch_gpio_config(int gpio, int level, unsigned int gpio_base_addr)
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

static int nct_get_kind(struct chip *chip)
{
	if (strncmp(chip->name, "NCT6776", 7) == 0)
		chip->kind = NCT6776;
	else if (strcmp(chip->name, "NCT6779D") == 0)
		chip->kind = NCT6779D;
	else if (strcmp(chip->name, "NCT5533D") == 0)
		chip->kind = NCT5533D;
	else {
		ERR("%s: Unsupport Chip: %s!\n", __func__, chip->name);
		return -1;
	}

	return 0;
}

static int sio_gpio_config(int gpio, int level, struct chip *chip)
{
	int index, ldn, offset;

	/* Get chip.kind */
	if (nct_get_kind(chip))
		return -1;

	/* setup multi-function pin for NCT5533D, this should be done by BIOS. */
	if (strncmp(chip->name, "NCT5", 4) == 0) {
		nct5xxx_multi_func_pin(gpio);
	}

	/* Get GPIO enable logical device number */
	ldn = sio_gpio_get_en_ldn(chip->name, gpio);
	offset = sio_gpio_get_en_offset(chip->name, gpio);
	DBG("gpio = %d, enldn = %d, offset = %d\n", gpio, ldn, offset);
	if (ldn == -1 || offset == -1) {
		ERR("ldn = %d, offset = %d, %s not support or GPIO%d incorrect!\n", \
				ldn, offset, chip, gpio);
		exit(1);
	}

	sio_gpio_enable(ldn, offset);
	ldn = sio_gpio_get_ldn(chip->name, gpio);
	sio_select(ldn);
	index = sio_get_gpio_dir_index(chip->name, gpio);
	DBG("ldn = %d, index = %x\n", ldn, index);
	if (index == -1) {
		ERR("GPIO%d incorrect!\n", gpio);
		exit(1);
	}

	sio_gpio_dir_out(gpio, level, index, NCT_GPIO_OUT);

	printf("Set GPIO[%d] Level to %s\n", gpio, level ? "HIGH" : "LOW");
}

