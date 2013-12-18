/* GPIO loop back testing.
 * Support S0211, S0981, S0961 with PCH, and S0361 with SuperIO.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>
#include <gpio-loopback.h>
#include <pchlib.h>
#include <siolib.h>

struct board_list
{
	char name[10];
	unsigned long int base_addr;
};

struct board_list list[] = {
	{ "S0981", 0x1C00 },
	{ "S0961", 0x1C00 },
	{ "S0211", 0x500 },
	{ "S0361", 0 }
};

unsigned long int base_addr = 0;

static void gpio_set_then_read(int gpio_out, int gpio_in, int value);
static void sio_gpio_set_then_read(int gpio_out, int gpio_in, int value);
static int sio_gpio_calculate(int gpio);

/* Set gpio_out direction to output and read from gpio_in */
static void gpio_set_then_read(int gpio_out, int gpio_in, int value)
{
	unsigned long int gpio_use_sel_addr, gp_io_sel_addr, gp_lvl_addr;
	int new_gpio;

	/* Set gpio_out direction to output and pull low or high. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio_out, base_addr);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_out(gp_io_sel_addr, gp_lvl_addr, new_gpio, value);

	/* Set gpio_in direction to input and read data. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio_in, base_addr);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_in(gp_io_sel_addr, new_gpio);

	printf("GPIO[%d] output %d to GPIO[%d] test ", gpio_out, value, gpio_in);
	if (gpio_get(gp_lvl_addr, new_gpio) != value)
		printf("FAIL!\n");
	else
		printf("PASS!\n");
}

static int sio_gpio_calculate(int gpio)
{
	if (gpio >= 70 && gpio <= 77)
		return (gpio - 70);
	else {
		printf("GPIO number incorrect.\n");
		exit(-1);
	}
}

static void sio_gpio_set_then_read(int gpio_out, int gpio_in, int value)
{
	int new_gpio;
	unsigned char b;

	/* Enable GPIO7 group */
	sio_gpio_enable(SIO_GPIO_EN_REG);
	/* Select GPIO7 */
	sio_select(SIO_GPIO7_LDN);

	new_gpio = sio_gpio_calculate(gpio_out);
	sio_gpio_dir_out(new_gpio, value);

	new_gpio = sio_gpio_calculate(gpio_in);
	sio_gpio_dir_in(new_gpio);

	printf("GPIO[%d] output %d to GPIO[%d] test ", gpio_out, value, gpio_in);
	if (sio_gpio_get(new_gpio) == value)
		printf("PASS!\n");
	else
		printf("FAIL\n");
}

static int check_board_list(char *file_name)
{
	int i;

	for (i = 0; i < (sizeof(list) / sizeof(list[0])); i++) {
		if (strncmp(file_name, list[i].name, 5) == 0) {
			base_addr = list[i].base_addr;
			return 0;
		}
	}

	return -1;
}

int main(int argc, char *argv[])
{
	int i, total, pin1, pin2;
	FILE *fp;

	/* Change I/O privilege level to all access. For Linux only. 
	 * If has error, show error message. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	if (argc != 2) {
		printf("Usage: COMMAND FILE_NAME\n");
		exit(-1);
	}

	if ((fp = fopen(argv[1], "r")) == NULL) {
		printf("No such file, please enter the correct file name.\n");
		printf("Usage: COMMAND FILE_NAME\n");
		exit(-1);
	}

	if (check_board_list(argv[1]) != 0) {
		printf("We don't support this board yet.\n");
		exit(-1);
	}

	/* Skip the header */
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);

	fscanf(fp, "%d\n", &total);

	for (i = 0; i < total; i += 2) {
		fscanf(fp, "%d %d", &pin1, &pin2);
		printf("Testing GPIO[%d] and GPIO[%d]...\n", pin1, pin2);

		if (strncmp(argv[1], "S0981", 5) == 0 || \
            strncmp(argv[1], "S0961", 5) == 0 || \
            strncmp(argv[1], "S0211", 5) == 0) {
			/* gpio_set_then_read(out, in, value) */
			gpio_set_then_read(pin1, pin2, GPIO_LOW);
			gpio_set_then_read(pin1, pin2, GPIO_HIGH);
			gpio_set_then_read(pin2, pin1, GPIO_LOW);
			gpio_set_then_read(pin2, pin1, GPIO_HIGH);
		} else {
			sio_enter();
			sio_gpio_set_then_read(pin1, pin2, GPIO_LOW);
			sio_gpio_set_then_read(pin1, pin2, GPIO_HIGH);
			sio_gpio_set_then_read(pin2, pin1, GPIO_LOW);
			sio_gpio_set_then_read(pin2, pin1, GPIO_HIGH);
			sio_exit();
		}
	}

	fclose(fp);

	return 0;
}

