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

static int sio_gpio_set_then_read(int gpio_out, int gpio_in, int value, char *chip)
{
	int new_gpio, index;

	/* Enable GPIO7 group */
	sio_gpio_enable(NCT_GPIO7_EN_LDN, GPIO7);
	/* Select GPIO7 */
	sio_select(NCT_GPIO7_LDN);

	index = sio_get_gpio_dir_index(chip, gpio_out);
	sio_gpio_dir_out(gpio_out, value, index, NCT_GPIO_OUT);
	
	index = sio_get_gpio_dir_index(chip, gpio_out);
	sio_gpio_dir_in(gpio_in, index, NCT_GPIO_IN);

	printf("GPIO[%d] output %d to GPIO[%d] test ", gpio_out, value, gpio_in);
	if (sio_gpio_get(gpio_in, index) == value) {
		printf("PASS!\n");
		return 0;
	} else {
		printf("FAIL\n");
		return -1;
	}
}

int main(int argc, char *argv[])
{
	int i, total, pin1, pin2;
	FILE *fp;
	char chip[10];

	/* 
	 * Change I/O privilege level to all access. For Linux only. 
	 * If has error, show error message.
	 */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	if (argc != 2) {
		printf("Usage: COMMAND FILE_NAME\n");
		exit(-1);
	}

	if ((fp = fopen(argv[1], "r")) == NULL) {
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
			sio_enter(argv[1]);
			if (sio_gpio_set_then_read(pin1, pin2, GPIO_LOW, chip) == -1)
				exit(1);
			if (sio_gpio_set_then_read(pin1, pin2, GPIO_HIGH, chip) == -1)
				exit(1);
			if (sio_gpio_set_then_read(pin2, pin1, GPIO_LOW, chip) == -1)
				exit(1);
			if (sio_gpio_set_then_read(pin2, pin1, GPIO_HIGH, chip) == -1)
				exit(1);
			sio_exit();
		}
	}

	fclose(fp);

	return 0;
}

