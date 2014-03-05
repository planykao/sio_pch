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

#define PAIR_GPIO_NUM 3
#define AST_GPIO_NUM  19
#define CPLD_DELAY    500000 /* micro second */

struct cpld_cfg
{
	int pair_g[12];
	int cfg_g[9];
	int sendbit[3];
};

struct ast_cpld_cfg
{
	char pair_g[12][3];
	char cfg_g[3][3];
	char sendbit[3];
};

struct gpio_groups
{
	char name;
	unsigned long int data_offset;
	unsigned long int dir_offset;
};

void usage(void);

void set_gpio(int gpio,int value);
void pair_setup(int pair, struct cpld_cfg *cpld);
void cfg_setup(int pair, int on, int off, int wdt, struct cpld_cfg *cpld);
void cpld_trigger(int pair, struct cpld_cfg *cpld);
void bypass_setup(int pair,int on, int off, int wdt, struct cpld_cfg *cpld);

void sio_pair_setup(int pair, struct cpld_cfg *cpld);
void sio_cfg_setup(int pair, int on, int off, int wdt, struct cpld_cfg *cpld);
void sio_cpld_trigger(int pair, struct cpld_cfg *cpld);
void sio_bypass_setup(int pair,int on, int off, int wdt, struct cpld_cfg *cpld);
//int f71889ad_get_gpio_dir_index(int gpio);
void f71889ad_gpio_dir_out(int gpio, int value);

int ast_get_gpio_offset(char *pair_g, struct gpio_groups *gg);
int ast_gpio_init(struct ast_cpld_cfg *cpld, struct gpio_groups *gg);
void ast_pair_setup(int value, unsigned long int data_offset, int offset);
void ast_cfg_setup(int value, unsigned long int data_offset, int offset);
void ast_cpld_trigger(unsigned long int data_offset, int offset);
int ast_bypass_setup(int pair, int cfg, struct ast_cpld_cfg *cpld, \
                      struct gpio_groups *gg);

int check_arguments(char *argv[], int *, int *, int *, int *);
int read_config(char *, struct cpld_cfg *, struct ast_cpld_cfg *, char *);

unsigned long int gpio_base_addr;
static int total_pair_pin_num;
static int total_cfg_pin_num = 3;

int main(int argc, char *argv[])
{
	unsigned char b;
	int pair, on, off, wdt, cfg;
	char chip[10];
	struct cpld_cfg *cpld;
	struct ast_cpld_cfg *ast_cpld;

	if (argc < 6) {
		usage();
		exit(-1);
	}

	if (check_arguments(argv, &pair, &on, &off, &wdt)) {
		ERR("Incorrect arguments!\n");
		usage();
		exit(-1);
	}

	cpld = malloc(sizeof(struct cpld_cfg));
	ast_cpld = malloc(sizeof(struct ast_cpld_cfg));

	if (read_config(argv[1], cpld, ast_cpld, chip)) {
		ERR("Fail to open <%s>, please enter the correct file name.\n", argv[1]);
		free(cpld);
		exit(-1);
	}

	DBG("pair = %d, on = %d, off = %d, wdt = %d\n", pair, on, off, wdt);

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	if (strcmp(chip, "PCH") == 0) {
		bypass_setup(pair, on, off, wdt, cpld);
	} else if (strncmp(chip, "F71", 3) == 0 || strncmp(chip, "AST", 3) == 0) {
		/* Assign EFER and EFDR for SuperIO */
		EFER = gpio_base_addr;
		EFDR = gpio_base_addr + 1;

		/* Enter the Extended Function Mode */
		sio_enter(chip);

		if (strncmp(chip, "AST", 3) == 0) {
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

			if (ast_gpio_init(ast_cpld, gg))
				ERR("ast_gpio_init fail.\n");

			/*
			 * ast_bypass_setup(..., int cfg, ...), 
			 * cfg = wdt | off << 1 | on << 2
			 * cfg_g[0] --> CFG1: WDT, 0 for Reset       , 1 for Bypass.
			 * cfg_g[1] --> CFG2: OFF, 0 for Pass Through, 1 for Bypass.
			 * cfg_g[2] --> CFG3: ON , 0 for Pass Through, 1 for Bypass.
			 */
			if (ast_bypass_setup(pair, wdt | off << 1 | on << 2, ast_cpld, gg))
				ERR("ast_bypass_setup fail.\n");
		} else if (strncmp(chip, "F71", 3) == 0) {
			sio_select(FINTEK_GPIO_LDN);
			sio_bypass_setup(pair, on, off, wdt, cpld);
		} else {
			ERR("this program doesn't support <%s> yet.", chip);
		}

		sio_exit();
	} else {
		ERR("%s is illegal name.\n", chip);
	}

	return 0;
}

int check_arguments(char *argv[], int *pair, int *on, int *off, int *wdt)
{
	int i;

	if (atoi(argv[2]) < 1 || atoi(argv[2]) > 8) {
		ERR("Pair should >= 1 and <= 8\n");
		return 1;
	}

	for (i = 3; i <= 5; i++) {
		if (atoi(argv[i]) < 0 || atoi(argv[i]) > 1) {
			ERR("PWRON/PWROFF/WDT should be 0 or 1\n");
			return 1;
		}
	}

	*pair = atoi(argv[2]);
	*on   = atoi(argv[3]);
	*off  = atoi(argv[4]);
	*wdt  = atoi(argv[5]);

	return 0;
}

int read_config(char *filename, struct cpld_cfg *cpld, \
                struct ast_cpld_cfg *ast_cpld, char *chip)
{
	FILE *fp;
	int i;
	char number[3][3];

	fp = fopen(filename, "r");

	if (fp == NULL)
		return 1;

	DBG("filename: %s\n", filename);

	if (strncmp(filename, "S0961", 5) == 0)
		total_pair_pin_num = 9;
	else
		total_pair_pin_num = 3;

	/* Skip the header */
	for (i = 0; i < 5; i++)
		fscanf(fp, "%*[^\n]\n", NULL);

	fscanf(fp, "%[^,], %x\n", chip, &gpio_base_addr);
	DBG("chip: %s, gpio_base_addr = %x\n", chip, gpio_base_addr);

	if (strncmp(chip, "AST", 3) == 0) {
		free(cpld);
		fscanf(fp, "%[^,], %[^,], %[^\n]\n", ast_cpld->pair_g[0], \
               ast_cpld->pair_g[1], ast_cpld->pair_g[2]);
		fscanf(fp, "%[^,], %[^,], %[^\n]\n", ast_cpld->cfg_g[0], \
               ast_cpld->cfg_g[1], ast_cpld->cfg_g[2]);
		fscanf(fp, "%s", ast_cpld->sendbit);

#ifdef DEBUG
		DBG("\n");
		printf("================GPIOs================\n");
		for (i = 0; i < total_pair_pin_num; i++)
			printf("%s ", ast_cpld->pair_g[i]);
		printf("\n");
		for (i = 0; i < total_pair_pin_num; i++)
			printf("%s ", ast_cpld->cfg_g[i]);
		printf("\n");
		for (i = 0; i < total_pair_pin_num / 3; i++)
			printf("%s ", cpld->sendbit);
		printf("=====================================\n");
#endif
	} else {
		free(ast_cpld);

		if (strncmp(filename, "S0961", 5) == 0) {
			fscanf(fp, "%d, %d, %d, %d, %d, %d, %d, %d, %d\n", \
                   &cpld->pair_g[0], &cpld->pair_g[1], &cpld->pair_g[2], \
                   &cpld->pair_g[3], &cpld->pair_g[4], &cpld->pair_g[5], \
                   &cpld->pair_g[6], &cpld->pair_g[7], &cpld->pair_g[8]);
			fscanf(fp, "%d, %d, %d, %d, %d, %d, %d, %d, %d\n", \
                   &cpld->cfg_g[0], &cpld->cfg_g[1], &cpld->cfg_g[2], \
                   &cpld->cfg_g[3], &cpld->cfg_g[4], &cpld->cfg_g[5], \
                   &cpld->cfg_g[6], &cpld->cfg_g[7], &cpld->cfg_g[8]);
			fscanf(fp, "%d, %d, %d\n", &cpld->sendbit[0], &cpld->sendbit[1], \
                                       &cpld->sendbit[2]);
		} else {
			fscanf(fp, "%d, %d, %d\n", &cpld->pair_g[0], &cpld->pair_g[1], \
                                       &cpld->pair_g[2]);
			fscanf(fp, "%d, %d, %d\n", &cpld->cfg_g[0], &cpld->cfg_g[1], \
                                       &cpld->cfg_g[2]);
			fscanf(fp, "%d\n", &cpld->sendbit[0]);
		}

#ifdef DEBUG
		DBG("\n");
		printf("================GPIOs================\n");
		for (i = 0; i < total_pair_pin_num; i++)
			printf("%d ", cpld->pair_g[i]);
		printf("\n");
		for (i = 0; i < total_pair_pin_num; i++)
			printf("%d ", cpld->cfg_g[i]);
		printf("\n");
		for (i = 0; i < total_pair_pin_num / 3; i++)
			printf("%d ", cpld->sendbit[i]);
		printf("\n");
		printf("=====================================\n");
#endif
	}

	fclose(fp);

	return 0;
}

int ast_get_gpio_offset(char *gpio, struct gpio_groups *gg)
{
	int offset;

	DBG("gpio = %s\n", gpio);
	for (offset = 0; offset < AST_GPIO_NUM; offset++) {
		if (strncmp(gpio, &gg[offset].name, 1) == 0)
			break;
	}

	if (offset == AST_GPIO_NUM)
		return -1;
	else
		return offset;
}

/*
 * ast_gpio_init(), initial GPIOs and set the direction to output.
 */
int ast_gpio_init(struct ast_cpld_cfg *cpld, struct gpio_groups *gg)
{
	unsigned char data;
	int i, ret, offset;

	/* Setup GPIOs for Pair */
	for (i = 0; i < total_pair_pin_num; i++) {
		ret = ast_get_gpio_offset(cpld->pair_g[i], gg);

		if (ret == -1) {
			ERR("fail to get gpio offset for %s.\n", cpld->pair_g[i]);
			return 1;
		}

		offset = atoi(cpld->pair_g[i] + 1);
		DBG("data_offset = %x, dir_offset = %x, offset = %d\n", \
            gg[ret].data_offset, gg[ret].dir_offset, offset);
	
		/* 
		 * set gpio direction 
		 * 0x1: output
         * 0x0: input
		 */
		data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + gg[ret].dir_offset, \
                                 HIGH_GPIO_BASE_ADDR);
		data |= (0x1 << offset);
		sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + gg[ret].dir_offset, \
                           HIGH_GPIO_BASE_ADDR);
	}

	/* Setup GPIOs for CFG */
	for (i = 0; i < total_cfg_pin_num; i++) {
		ret = ast_get_gpio_offset(cpld->cfg_g[i], gg);

		if (ret == -1) {
			ERR("fail to get gpio offset for %s.\n", cpld->cfg_g[i]);
			return 1;
		}

		offset = atoi(cpld->cfg_g[i] + 1);
		DBG("data_offset = %x, dir_offset = %x, offset = %d\n", \
            gg[ret].data_offset, gg[ret].dir_offset, offset);

		/* 
		 * set gpio direction 
		 * 0x1: output
         * 0x0: input
		 */
		data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + gg[ret].dir_offset, \
                                 HIGH_GPIO_BASE_ADDR);
		data |= (0x1 << offset);
		sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + gg[ret].dir_offset, \
                           HIGH_GPIO_BASE_ADDR);
	}

	/* Setup GPIO for sendbit */
	ret = ast_get_gpio_offset(cpld->sendbit, gg);

	if (ret == -1) {
		ERR("fail to get gpio offset for %s.\n", cpld->sendbit);
		return 1;
	}

	offset = atoi(cpld->sendbit + 1);
	DBG("data_offset = %x, dir_offset = %x, offset = %d\n", \
        gg[ret].data_offset, gg[ret].dir_offset, offset);

	data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + gg[ret].dir_offset, \
                             HIGH_GPIO_BASE_ADDR);
	data |= (0x1 << offset);
	sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + gg[ret].dir_offset, \
                       HIGH_GPIO_BASE_ADDR);

	return 0;
}

/*
 * ast_pair_setup(), setup the Pair.
 */
void ast_pair_setup(int value, unsigned long int data_offset, int offset)
{
	int data;

	DBG("value = %d\n", value);

	data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	data &= ~(0x1 << offset);
	data |= (value << offset);
	sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + data_offset, \
                       HIGH_GPIO_BASE_ADDR);
}

/*
 * ast_cfg_setup(). setup the CFG.
 */
void ast_cfg_setup(int value, unsigned long int data_offset, int offset)
{
	int data;

	DBG("value = %d\n", value);

	data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	data &= ~(0x1 << offset);
	data |= (value << offset);
	sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + data_offset, \
                       HIGH_GPIO_BASE_ADDR);
}

/*
 * ast_cpld_trigger(), trigger the sendbit.
 */
void ast_cpld_trigger(unsigned long int data_offset, int offset)
{
	int data;

	data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	data &= ~(0x1 << offset);
	data |= (GPIO_LOW << offset);
	sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + data_offset, \
                       HIGH_GPIO_BASE_ADDR);

	usleep(CPLD_DELAY);

	data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	data &= ~(0x1 << offset);
	data |= (GPIO_HIGH << offset);
	sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + data_offset, \
                       HIGH_GPIO_BASE_ADDR);

	usleep(CPLD_DELAY);

	data = sio_ilpc2ahb_read(LOW_GPIO_BASE_ADDR + data_offset, \
                             HIGH_GPIO_BASE_ADDR);
	data &= ~(0x1 << offset);
	data |= (GPIO_LOW << offset);
	sio_ilpc2ahb_write(data, LOW_GPIO_BASE_ADDR + data_offset, \
                       HIGH_GPIO_BASE_ADDR);
}

int ast_bypass_setup(int pair, int cfg, struct ast_cpld_cfg *cpld, \
                     struct gpio_groups *gg)
{
	unsigned char data;
	int i, ret, offset;

	/* pair */
	for (i = 0; i < total_pair_pin_num; i++) {
		ret = ast_get_gpio_offset(cpld->pair_g[i], gg);

		if (ret == -1) {
			ERR("fail to get gpio offset for %s.\n", cpld->pair_g[i]);
			return 1;
		}

		offset = atoi(cpld->pair_g[i] + 1);
		ast_pair_setup((pair - 1 >> i) & 0x1, gg[ret].data_offset, offset);
	}

	/* cfg */
	DBG("cfg = %d\n", cfg);

	for (i = 0; i < total_cfg_pin_num; i++) {
		ret = ast_get_gpio_offset(cpld->cfg_g[i], gg);

		if (ret == -1) {
			ERR("fail to get gpio offset for %s.\n", cpld->cfg_g[i]);
			return 1;
		}

		offset = atoi(cpld->cfg_g[i] + 1);

		/*
		 * cfg_g[0] --> CFG1: Watch Dog Timer, 0 for Reset       , 1 for Bypass.
		 * cfg_g[1] --> CFG2: OFF            , 0 for Pass Through, 1 for Bypass.
		 * cfg_g[2] --> CFG3: ON             , 0 for Pass Through, 1 for Bypass.
		 */
		ast_cfg_setup((cfg >> i) & 0x1, gg[ret].data_offset, offset);
	}

	/* trigger */
	ret = ast_get_gpio_offset(cpld->sendbit, gg);

	if (ret == -1) {
		ERR("fail to get gpio offset for %s.\n", cpld->sendbit);
		return 1;
	}

	offset = atoi(cpld->sendbit + 1);
	ast_cpld_trigger(gg[ret].data_offset, offset);

	return 0;
}

/*
 * TODO: implement a gpio_init() function to initial all GPIOs and set these 
 *       GPIOs to output in gpio_init(). And then use gpio_set() to replace 
 *       set_gpio() to make the code more readable.
 */

/*
 * set_gpio(), set gpio_out direction to output and pull low or high.
 */
void set_gpio(int gpio, int value)
{
	unsigned long int gpio_use_sel_addr, gp_io_sel_addr, gp_lvl_addr;
	int new_gpio;

	DBG("gpio = %d, value = %d\n", gpio, value);
	/* Set gpio_out direction to output and pull low or high. */
	new_gpio = gpio_setup_addr(&gpio_use_sel_addr, &gp_io_sel_addr, \
                               &gp_lvl_addr, gpio, gpio_base_addr);
	DBG("gpio_use_sel_addr = %x, new_gpio = %d\n", gpio_use_sel_addr, new_gpio);
	
	gpio_enable(gpio_use_sel_addr, new_gpio);
	DBG("gpio_sel_addr = %x, gp_lvl_addr =%x, value = %d\n", \
         gp_io_sel_addr, gp_lvl_addr, value);
	
	gpio_dir_out(gp_io_sel_addr, gp_lvl_addr, new_gpio, value);
}

/*
 * pair_setup(), setup the Pair.
 *
 * pair_g[0] --> Pair1
 * pair_g[1] --> Pair2
 * pair_g[2] --> Pair3
 *
 * If pair = 2, d'2 -> b'010, bit0 is 0, bit1 is 1, bit2 is 0, 
 * so pair_g[2] = 0 
 *    pair_g[1] = 1 
 *    pair_g[0] = 0
 *
 * if pair = 3, d'5 -> b'011, bit0 is 1, bit1 is 1, bit2 is 0, 
 * so pair_g[2] = 0
 *    pair_g[1] = 1
 *    pair+g[0] = 1.
 */
void pair_setup(int pair, struct cpld_cfg *cpld)
{
	int i, n = 0;

	pair -= 1; /* make pair from 1 ~ 8 to 0 ~ 7 */

	/* For S0961, it use different GPIOs for Pair1/2, Pair3/4 and Pair5/6 */
	if (total_pair_pin_num == 9) {
		n = pair / 2;
		DBG("n = %d\n", n);
	}

	for (i = 0; i < PAIR_GPIO_NUM; i++)
		set_gpio(cpld->pair_g[n * PAIR_GPIO_NUM + i], (pair >> i) & 0x1);
}

/*
 * cfg_setup(), setup the CFG.
 *
 * cfg_g[0] --> CFG1: Watch Dog Timer, 0 for Reset       , 1 for Bypass.
 * cfg_g[1] --> CFG2: OFF            , 0 for Pass Through, 1 for Bypass.
 * cfg_g[2] --> CFG3: ON             , 0 for Pass Through, 1 for Bypass.
 */
void cfg_setup(int pair, int on, int off, int wdt, struct cpld_cfg *cpld)
{
	int n = 0;

	if (total_pair_pin_num == 9) {
		n = ((pair - 1) / 2) * 3;
		DBG("n = %d\n", n);
	}

	set_gpio(cpld->cfg_g[n + 0], wdt);
	set_gpio(cpld->cfg_g[n + 1], off);
	set_gpio(cpld->cfg_g[n + 2], on);
}

/*
 * cpld_trigger(), trigger the sendbit.
 *
 * The sendbit is falling-edge trigger, the delay time must 
 * longer than 100ms.
 */
void cpld_trigger(int pair, struct cpld_cfg *cpld)
{
	int n = 0;
	
	if (total_pair_pin_num == 9) {
		n = (pair - 1) / 2;
		DBG("n = %d\n", n);
	}

	set_gpio(cpld->sendbit[n], GPIO_LOW);
	usleep(CPLD_DELAY);
	set_gpio(cpld->sendbit[n], GPIO_HIGH);
	usleep(CPLD_DELAY);
	set_gpio(cpld->sendbit[n], GPIO_LOW);
}

void bypass_setup(int pair, int on, int off, int wdt, struct cpld_cfg *cpld)
{
	pair_setup(pair, cpld);
	cfg_setup(pair, on, off, wdt, cpld);
	cpld_trigger(pair, cpld);
}

/*
 * sio_pair_setup(), setup the Pair.
 *
 * pair_g[0] --> Pair1
 * pair_g[1] --> Pair2
 * pair_g[2] --> Pair3
 *
 * If pair = 2, d'2 -> b'010, bit0 is 0, bit1 is 1, bit2 is 0, 
 * so pair_g[2] = 0 
 *    pair_g[1] = 1 
 *    pair_g[0] = 0
 *
 * if pair = 3, d'5 -> b'011, bit0 is 1, bit1 is 1, bit2 is 0, 
 * so pair_g[2] = 0
 *    pair_g[1] = 1
 *    pair+g[0] = 1.
 */
void sio_pair_setup(int pair, struct cpld_cfg *cpld)
{
	int i, n = 0;

	pair -= 1; /* make pair from 1 ~ 8 to 0 ~ 7 */

	/* For S0961, it use different GPIOs for Pair1/2, Pair3/4 and Pair5/6 */
	if (total_pair_pin_num == 9) {
		n = pair / 2;
		DBG("n = %d\n", n);
	}

	DBG("\n");
	for (i = 0; i < PAIR_GPIO_NUM; i++)
		f71889ad_gpio_dir_out(cpld->pair_g[n * PAIR_GPIO_NUM + i], \
                              (pair >> i) & 0x1);
}

/*
 * sio_cfg_setup(), setup the CFG.
 *
 * cfg_g[0] --> CFG1: Watch Dog Timer, 0 for Reset       , 1 for Bypass.
 * cfg_g[1] --> CFG2: OFF            , 0 for Pass Through, 1 for Bypass.
 * cfg_g[2] --> CFG3: ON             , 0 for Pass Through, 1 for Bypass.
 */
void sio_cfg_setup(int pair, int on, int off, int wdt, struct cpld_cfg *cpld)
{
	int n = 0;

	if (total_pair_pin_num == 9) {
		n = ((pair - 1) / 2) * 3;
		DBG("n = %d\n", n);
	}

	DBG("\n");
	f71889ad_gpio_dir_out(cpld->cfg_g[n + 0], wdt);
	f71889ad_gpio_dir_out(cpld->cfg_g[n + 1], off);
	f71889ad_gpio_dir_out(cpld->cfg_g[n + 2], on);
}

/*
 * sio_cpld_trigger(), trigger the sendbit.
 *
 * The sendbit is falling-edge trigger, the delay time must 
 * longer than 100ms.
 */
void sio_cpld_trigger(int pair, struct cpld_cfg *cpld)
{
	int n = 0;
	
	if (total_pair_pin_num == 9) {
		n = (pair - 1) / 2;
		DBG("n = %d\n", n);
	}

	DBG("\n");
	f71889ad_gpio_dir_out(cpld->sendbit[n], GPIO_LOW);
	usleep(CPLD_DELAY);
	f71889ad_gpio_dir_out(cpld->sendbit[n], GPIO_HIGH);
	usleep(CPLD_DELAY);
	f71889ad_gpio_dir_out(cpld->sendbit[n], GPIO_LOW);
}

void sio_bypass_setup(int pair, int on, int off, int wdt, struct cpld_cfg *cpld)
{
	sio_pair_setup(pair, cpld);
	sio_cfg_setup(pair, on, off, wdt, cpld);
	sio_cpld_trigger(pair, cpld);
}

#if 0
/* get gpio direction index, gpio data index = direction index + 1 */
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
}
#endif

/* TODO: this function needs to merge to sio_gpio_dir_out() in libsio.c */
void f71889ad_gpio_dir_out(int gpio, int value)
{
	int buf, dir_index;

	dir_index = f71889ad_get_gpio_dir_index(gpio);
	DBG("dir_index = %x\n", dir_index);

	/* set direction */
	buf = sio_read(dir_index);
	buf |= (0x1 << (gpio % 10));
	sio_write(dir_index, buf);

	/* set data */
	buf = sio_read(dir_index + 1);
	DBG("buf = %x\n", buf);
	buf &= ~(0x1 << (gpio % 10));
	DBG("value = %d, offset = %d\n", value, gpio % 10);
	buf |= (value << (gpio % 10));
	sio_write(dir_index + 1, buf);
}

void usage(void)
{
	printf("Usage : Command [FileName] [Pair] [PWRON] [PWROFF] [WDT]\n"
			" Pair : 1 and 2 for Slot1\n"
			"        3 and 4 for Slot2\n"
			"        5 and 6 for Slot3\n"
			"        7 and 8 for Slot4\n"
			" PWRON Status : 0(Passthru), 1(Bypass)\n"
			" PWROFF Status : 0(Passthru), 1(Bypass)\n"
			" WDT trigger : 0(reset), 1(bypass)\n");
}
