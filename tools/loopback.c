#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <sitest.h>
#include <libpch.h>
#include <libsio.h>

static int gpio_set_then_read(int gpio_out, int gpio_in, int value);
static int sio_gpio_set_then_read(int gpio_out, int gpio_in, int value, char *chip);
static void help(void);

extern void initcheck(void);

unsigned long int base_addr;

/* Set gpio_out direction to output and read from gpio_in */
static int gpio_set_then_read(int gpio_out, int gpio_in, int value)
{
	unsigned long int gpio_use_sel_addr, gp_io_sel_addr, gp_lvl_addr;
	int new_gpio;

	/* Set gpio_out direction to output and pull low or high. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio_out, base_addr);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_out(gp_io_sel_addr, gp_lvl_addr, new_gpio, value);

	DBG("gpio_sel_addr = %x, gp_lvl_addr =%x\n", gp_io_sel_addr, gp_lvl_addr);
	/* Set gpio_in direction to input and read data. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio_in, base_addr);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_in(gp_io_sel_addr, new_gpio);
	DBG("gpio_sel_addr = %x, gp_lvl_addr =%x\n", gp_io_sel_addr, gp_lvl_addr);

	printf("GPIO[%d] output %d to GPIO[%d] test ", gpio_out, value, gpio_in);
	if (gpio_get(gp_lvl_addr, gpio_in) == value) {
		printf("PASS!\n");
		return 0;
	} else {
		printf("FAIL!\n");
		return -1;
	}
}

static int sio_gpio_set_then_read(int gpio_out, int gpio_in, \
                                  int value, char *chip)
{
	int index, ldn, offset;

#if 1
	/* setup multi-function pin for NCT5533D, this should be done by BIOS. */
	if (strncmp(chip, "NCT5", 4) == 0) {
		nct5xxx_multi_func_pin(gpio_out);
		nct5xxx_multi_func_pin(gpio_in);
	}

	/* TODO: get gpio enable logical number from different sio */
	ldn = sio_gpio_get_en_ldn(chip, gpio_out);
	offset = sio_gpio_get_en_offset(chip, gpio_out);
	DBG("gpio_out = %d, enldn = %d, offset = %d\n", gpio_out, ldn, offset);
	if (ldn == -1 || offset == -1) {
		ERR("ldn = %d, offset = %d, %s not support or GPIO%d incorrect!\n", \
				ldn, offset, chip, gpio_in);
		exit(1);
	}

	sio_gpio_enable(ldn, offset);
	ldn = sio_gpio_get_ldn(chip, gpio_out);
	sio_select(ldn);
	index = sio_get_gpio_dir_index(chip, gpio_out);
	DBG("ldn = %d, index = %x\n", ldn, index);
	if (index == -1) {
		ERR("GPIO%d incorrect!\n", gpio_out);
		exit(1);
	}

	sio_gpio_dir_out(gpio_out, value, index, NCT_GPIO_OUT);


	ldn = sio_gpio_get_en_ldn(chip, gpio_in);
	offset = sio_gpio_get_en_offset(chip, gpio_in);
	DBG("gpio_in = %d, enldn = %d, offset = %d\n", gpio_in, ldn, offset);
	if (ldn == -1 || offset == -1) {
		ERR("ldn = %d, offset = %d, %s not support or GPIO%d incorrect!\n", \
				ldn, offset, chip, gpio_in);
		exit(1);
	}

	sio_gpio_enable(ldn, offset);
	ldn = sio_gpio_get_ldn(chip, gpio_in);
	sio_select(ldn);
	index = sio_get_gpio_dir_index(chip, gpio_in);
	DBG("ldn = %d, index = %x\n", ldn, index);
	if (index == -1) {
		ERR("GPIO%d incorrect!\n", gpio_in);
		exit(1);
	}

	sio_gpio_dir_in(gpio_in, index, NCT_GPIO_IN);
#else
	/* Enable GPIO7 group */
	sio_gpio_enable(NCT_GPIO7_EN_LDN, GPIO7);
	/* Select GPIO7 */
	sio_select(NCT_GPIO7_LDN);

	index = sio_get_gpio_dir_index(chip, gpio_out);
	sio_gpio_dir_out(gpio_out, value, index, NCT_GPIO_OUT);
	
	index = sio_get_gpio_dir_index(chip, gpio_in);
	sio_gpio_dir_in(gpio_in, index, NCT_GPIO_IN);
#endif

	printf("GPIO[%d] output %d to GPIO[%d] test ", gpio_out, value, gpio_in);
	if (sio_gpio_get(gpio_in, index) == value) {
		printf("PASS!\n");
		return 0;
	} else {
		printf("FAIL\n");
		return -1;
	}
}

static void help(void)
{
	printf("Usage: ./loopback [-c config]\n");
	printf("Usage: ./loopback [-h]\n\n");
	printf("  -c, <config_file>  configuration file for target board\n");
	printf("  -h,                print this message and quit\n\n");
	exit(1);
}

int main(int argc, char *argv[])
{
	int i, total, pin1, pin2;
	FILE *fp;
	char chip[10], args, *filename;

	initcheck();

	/* 
	 * Change I/O privilege level to all access. For Linux only. 
	 * If has error, show error message.
	 */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	while ((args = getopt(argc, argv, "c:h")) != -1) {
		switch (args) {
			case 'c':
				filename = optarg;
				break;
			case ':':
			case 'h':
			case '?':
				help();
				break;
		}
	}

	if (argc != 3)
		help();

	if ((fp = fopen(filename, "r")) == NULL) {
		ERR("Fail to open <%s>, please enter the correct file name.\n", argv[1]);
		printf("Usage: COMMAND FILE_NAME\n");
		exit(-1);
	}

	/* Skip the header */
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);

	/* Read chip set and gpio base addr */
	fscanf(fp, "%s %x", chip, &base_addr);

	/* Assign EFER and EFDR for SuperIO */
	if (strncmp("PCH", chip, 3) != 0) {
		EFER = base_addr;
		EFDR = EFER + 1;
	}

	DBG("chip = %s, base_addr = %x, EFER = %x, EFDR = %x\n", \
         chip, base_addr, EFER, EFDR);

	fscanf(fp, "%d\n", &total);

	DBG("total = %d\n", total);

	for (i = 0; i < total; i += 2) {
		fscanf(fp, "%d %d", &pin1, &pin2);
		printf("Testing GPIO[%d] and GPIO[%d]...\n", pin1, pin2);

		if (strcmp("PCH", chip) == 0) { /* test gpio via PCH */
			if (gpio_set_then_read(pin1, pin2, GPIO_LOW) == -1)
				exit(1);
			if (gpio_set_then_read(pin1, pin2, GPIO_HIGH) == -1)
				exit(1);
			if (gpio_set_then_read(pin2, pin1, GPIO_LOW) == -1)
				exit(1);
			if (gpio_set_then_read(pin2, pin1, GPIO_HIGH) == -1)
				exit(1);
		} else { /* test gpio via SIO */
//			sio_enter(argv[1]);
			sio_enter(chip);
#ifndef DEBUG
			if (sio_gpio_set_then_read(pin1, pin2, GPIO_LOW, chip) == -1)
				exit(1);
			if (sio_gpio_set_then_read(pin1, pin2, GPIO_HIGH, chip) == -1)
				exit(1);
			if (sio_gpio_set_then_read(pin2, pin1, GPIO_LOW, chip) == -1)
				exit(1);
			if (sio_gpio_set_then_read(pin2, pin1, GPIO_HIGH, chip) == -1)
				exit(1);
#else
			sio_gpio_set_then_read(pin1, pin2, GPIO_LOW, chip);
			printf("\n");
			sio_gpio_set_then_read(pin1, pin2, GPIO_HIGH, chip);
			printf("\n");
			sio_gpio_set_then_read(pin2, pin1, GPIO_LOW, chip);
			printf("\n");
			sio_gpio_set_then_read(pin2, pin1, GPIO_HIGH, chip); 
			printf("\n");
#endif
			sio_exit();
		}
	}

	fclose(fp);

	return 0;
}

