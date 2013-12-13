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

/* GPIO register address from PCH start */
#define GPIO_USE_SEL1 0x00 /* GPIO_USE_SEL1 offset */
#define GPIO_USE_SEL2 0x30 /* GPIO_USE_SEL2 offset */
#define GPIO_USE_SEL3 0x40 /* GPIO_USE_SEL3 offset */

#define GP_IO_SEL1 0x04 /* GPIO Input/Output Select1 offset */
#define GP_IO_SEL2 0x34 /* GPIO Input/Output Select2 offset */
#define GP_IO_SEL3 0x44 /* GPIO Input/Output Select3 offset */

#define GP_LVL1 0x0C /* GPIO Level1 for Input or Output offset */
#define GP_LVL2 0x38 /* GPIO Level2 for Input or Output offset */
#define GP_LVL3 0x48 /* GPIO Level3 for Input or Output offset */

#define GPIO_USE_SEL1_ADDR(addr) (addr + GPIO_USE_SEL1) 
#define GPIO_USE_SEL2_ADDR(addr) (addr + GPIO_USE_SEL2)
#define GPIO_USE_SEL3_ADDR(addr) (addr + GPIO_USE_SEL3)

#define GP_IO_SEL1_ADDR(addr) (addr + GP_IO_SEL1)
#define GP_IO_SEL2_ADDR(addr) (addr + GP_IO_SEL2)
#define GP_IO_SEL3_ADDR(addr) (addr + GP_IO_SEL3)

#define GP_LVL1_ADDR(addr) (addr + GP_LVL1)
#define GP_LVL2_ADDR(addr) (addr + GP_LVL2)
#define GP_LVL3_ADDR(addr) (addr + GP_LVL3)
/* GPIO register address from PCH end */

/* GPIO register address from SuperIO start*/
#define EFER                0x4E
#define EFDR                0x4F
#define SIO_LDSEL_REG       0x07
#define SIO_ENABLE_REG      0x30
#define SIO_GPIO_EN_REG     0x09
#define SIO_GPIO7_DIR_REG   0xE0
#define SIO_GPIO7_DATA_REG  0xE1
#define SIO_GPIO7_EN_OFFSET (0x1 << 7)
#define SIO_GPIO7_LDN       0x07
/* GPIO register address from SuperIO end*/

struct board_list
{
	char name[10];
	unsigned long int base_addr;
};

unsigned long int base_addr = 0;

/* Functions for GPIO from PCH */
static int gpio_setup_addr(unsigned long int *gpio_use_sel_addr, \
                            unsigned long int *gp_io_sel_addr, \
							unsigned long int *gp_lvl_addr, \
							int gpio);
static void gpio_enable(unsigned long int gpio_use_sel_addr, int gpio);
static unsigned long int gpio_get(unsigned long int gpio_lvl_addr, int gpio);
static void gpio_set(unsigned long int gpio_lvl_addr, int gpio, int value);
static void gpio_dir_in(unsigned long int gp_io_sel_addr, int gpio);
static void gpio_dir_out(unsigned long int gp_io_sel_addr, \
                         unsigned long int gp_lvl_addr, int gpio, int value);
static void gpio_set_then_read(int gpio_out, int gpio_in, int value);

static int gpio_setup_addr(unsigned long int *gpio_use_sel_addr, \
                            unsigned long int *gp_io_sel_addr, \
                            unsigned long int *gp_lvl_addr, \
							int gpio)
{
	int new_gpio;

	if (gpio >=1 && gpio <= 31) {
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
static void gpio_enable(unsigned long int gpio_use_sel_addr, int gpio)
{
	unsigned long int buf;
	
	buf = inl_p(gpio_use_sel_addr); /* Read the original value */
	buf |= (0x1 << gpio); /* Set the bit to 1 for GPIO mode */
	outl_p(buf, gpio_use_sel_addr); /* Output the value */
}

static unsigned long int gpio_get(unsigned long int gp_lvl_addr, int gpio)
{
	unsigned long int buf;

	buf = inl_p(gp_lvl_addr);
	buf = (buf >> gpio);
	buf &= 0x1;
	
	return buf;
}

static void gpio_set(unsigned long int gp_lvl_addr, int gpio, int value)
{
	unsigned long int buf;

	buf = inl_p(gp_lvl_addr); /* Read the original value */
	if (value == GPIO_LOW)
		buf &= ~(0x1 << gpio); /* Set the bit to 0 for low level */
	else
		buf |= (0x1 << gpio); /* Set the bit to 1 for high level */
	outl_p(buf, gp_lvl_addr); /* Output the value */
	printf("Set GPIO Level to %d\n", value);
}

static void gpio_dir_in(unsigned long int gp_io_sel_addr, int gpio)
{
	unsigned long int buf;

	buf = inl_p(gp_io_sel_addr); /* Read the original value */
	buf |= (0x1 << gpio); /* Set the bit to 0 for input */
	outl_p(buf, gp_io_sel_addr); /* Output the value */
}

static void gpio_dir_out(unsigned long int gp_io_sel_addr, \
                         unsigned long int gp_lvl_addr, int gpio, int value)
{
	unsigned long int buf;
	
	buf = inl_p(gp_io_sel_addr); /* Read the original value */
	buf &= ~(0x1 << gpio); /* Set the bit to 0 for output */
	outl_p(buf, gp_io_sel_addr); /* Output the value */

	buf = inl_p(gp_lvl_addr); /* Read the original value */
	if (value == GPIO_LOW)
		buf &= ~(0x1 << gpio); /* Set the bit to 0 for low level */
	else
		buf |= (0x1 << gpio); /* Set the bit to 1 for high level */
	outl_p(buf, gp_lvl_addr); /* Output the value */
	DBG("Set GPIO[%d] Level to %d\n", gpio, value);
}

/* Set gpio_out direction to output and read from gpio_in */
static void gpio_set_then_read(int gpio_out, int gpio_in, int value)
{
	unsigned long int gpio_use_sel_addr, gp_io_sel_addr, gp_lvl_addr;
	int new_gpio;

	/* Set gpio_out direction to output and pull low or high. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio_out);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_out(gp_io_sel_addr, gp_lvl_addr, new_gpio, value);

	/* Set gpio_in direction to input and read data. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio_in);
	gpio_enable(gpio_use_sel_addr, new_gpio);
	gpio_dir_in(gp_io_sel_addr, new_gpio);

	printf("GPIO[%d] output %d to GPIO[%d] test ", gpio_out, value, gpio_in);
	if (gpio_get(gp_lvl_addr, new_gpio) != value)
		printf("FAIL!\n");
	else
		printf("PASS!\n");
}
/* Functions for GPIO from PCH end */

/* Functions for GPIO from SuperIO start */
static void sio_gpio_enable(int ldnum);
static unsigned char sio_gpio_get(int gpio);
static void sio_gpio_set(int gpio, int value);
static void sio_gpio_dir_in(int gpio);
static void sio_gpio_dir_out(int gpio, int value);
static void sio_gpio_set_then_read(int gpio_out, int gpio_in, int value);
static int sio_gpio_calculate(int gpio);
static void sio_enter(void);
static void sio_exit(void);
static unsigned char sio_read(int reg);
static void sio_write(int reg, unsigned char val);
static void sio_select(int ldnum);

static void sio_enter(void)
{
	outb_p(0x87, EFER);
	outb_p(0x87, EFER);
}

static void sio_exit(void)
{
	outb_p(0xAA, EFER);
}

static unsigned char sio_read(int reg)
{
	unsigned char b;
	
	outb_p(reg, EFER); /* Sent index to EFER */
	b = inb_p(EFDR); /* Get the value from EFDR */
	return b; /* Return the value */
}

static void sio_write(int reg, unsigned char val)
{
	outb_p(reg, EFER); /* Sent index at EFER */
	outb_p(val, EFDR); /* Send val_w at FEDR */
}

/* ldsel_reg: Logical Device Select 
 * ldnum:     Logical Device Number */
static void sio_select(int ldnum)
{
	outb_p(SIO_LDSEL_REG, EFER);
	outb_p(ldnum, EFDR);
}

static void sio_gpio_enable(int ldnum)
{
	unsigned char b;
	
	/*sio_write(7, 9);*/ /* Select Logical device number 9 */
	/* CR 30h of Logical Device Number 9 is enable register for GPIO1~7 */
	sio_select(ldnum);

	/* Active GPIO7 Group */
	b = sio_read(SIO_ENABLE_REG); /* Read the value of CR 30h of Logical device 9 */
	DBG("b = %x\n", b);
	b |= SIO_GPIO7_EN_OFFSET; /* Set bit7 to 1 to enable GPIO7 Group */
	sio_write(SIO_ENABLE_REG, b); /* Write the value at CR 30h of Logical device 9 */
}

static unsigned char sio_gpio_get(int gpio) {
	return ((sio_read(SIO_GPIO7_DATA_REG) >> gpio) & 0x1);
}

static void sio_gpio_set(int gpio, int value)
{
	unsigned char b;

	b = sio_read(SIO_GPIO7_DATA_REG);
	b &= ~(0x1 << gpio); /* clear */
	b |= (value << gpio);
	sio_write(SIO_GPIO7_DATA_REG, b);
}

static void sio_gpio_dir_in(int gpio)
{
	unsigned char b;

	b = sio_read(SIO_GPIO7_DIR_REG);
	b |= (0x1 << gpio); /* set 1 for input */
	sio_write(SIO_GPIO7_DIR_REG, b);
}

static void sio_gpio_dir_out(int gpio, int value)
{
	unsigned char b;

	/* direction */
	b = sio_read(SIO_GPIO7_DIR_REG);
	b &= ~(0x1 << gpio); /* clear to 0 for output */
	sio_write(SIO_GPIO7_DIR_REG, b);

	/* data */
	sio_gpio_set(gpio, value);
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
/* Functions for GPIO from SuperIO end */

struct board_list list[] = {
	{ "S0981", 0x1C00 },
	{ "S0961", 0x1C00 },
	{ "S0211", 0x500 },
	{ "S0361", 0 }
};

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

