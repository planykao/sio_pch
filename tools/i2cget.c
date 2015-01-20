/*
    i2cget.c - A user-space program to read an I2C register.
    Copyright (C) 2005-2012  Jean Delvare <jdelvare@suse.de>

    Based on i2cset.c:
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2005  Jean Delvare

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <i2cbusses.h>
#include <util.h>
#include <sitest.h>
#include <curses.h>
#include <sys/time.h>

#define VERSION "3.1.1"

#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

struct sch5027_hwmon {
	char name[30];
	int offset;
	int min_offset;
	int max_offset;
	float t_min;
	float t_max;
	float min;
	float max;
	float cur;
	float scale;
	int volt_func;
	int error;
};

static void help(void) __attribute__ ((noreturn));

static void help(void)
{
	fprintf(stderr,
		"Usage: i2cget [-f] [-y] I2CBUS CHIP-ADDRESS [DATA-ADDRESS [MODE]]\n"
		"  I2CBUS is an integer or an I2C bus name\n"
		"  ADDRESS is an integer (0x03 - 0x77)\n"
		"  MODE is one of:\n"
		"    b (read byte data, default)\n"
		"    w (read word data)\n"
		"    c (write byte/read byte)\n"
		"    Append p for SMBus PEC\n");
	exit(1);
}

static int check_funcs(int file, int size, int daddress, int pec)
{
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	switch (size) {
	case I2C_SMBUS_BYTE:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus receive byte");
			return -1;
		}
		if (daddress >= 0
		 && !(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
			return -1;
		}
		break;

	case I2C_SMBUS_BYTE_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_BYTE_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read byte");
			return -1;
		}
		break;

	case I2C_SMBUS_WORD_DATA:
		if (!(funcs & I2C_FUNC_SMBUS_READ_WORD_DATA)) {
			fprintf(stderr, MISSING_FUNC_FMT, "SMBus read word");
			return -1;
		}
		break;
	}

	if (pec
	 && !(funcs & (I2C_FUNC_SMBUS_PEC | I2C_FUNC_I2C))) {
		fprintf(stderr, "Warning: Adapter does "
			"not seem to support PEC\n");
	}

	return 0;
}

extern unsigned int scan_pch_gpio_base(void);
extern int gpio_setup_addr(unsigned long int *gpio_use_sel_addr, \
                           unsigned long int *gp_io_sel_addr, \
                           unsigned long int *gp_lvl_addr, \
                           int gpio, unsigned long int base_addr);
extern void gpio_enable(unsigned long int gpio_use_sel_addr, int gpio);
extern void gpio_dir_out(unsigned long int gp_io_sel_addr, \
                         unsigned long int gp_lvl_addr, int gpio, int value);

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
}

static float read_sensor(int file, struct sch5027_hwmon *s)
{
	return i2c_smbus_read_byte_data(file, s->offset) \
			* (s->scale == -1 ? 1 : s->scale);
}

static float read_sensor_min(int file, struct sch5027_hwmon *s)
{
	return i2c_smbus_read_byte_data(file, s->min_offset) \
			* (s->scale == -1 ? 1 : s->scale);
}

static float read_sensor_max(int file, struct sch5027_hwmon *s)
{
	return i2c_smbus_read_byte_data(file, s->max_offset) \
			* (s->scale == -1 ? 1 : s->scale);
}

/* 
 * The FAN Tachometer Reading register contain the number of 11.111us 
 * periods (90KHz) between the programmed number of edges. Five edges 
 * returns the number of clock between full fan revolutions if the fans
 * produce two tachometer pulses per full revolution. These register are 
 * updated at least once every second.
 *
 * RPM = scale / ((reg + 1 << 8) | reg), where scale = 90000 * 60
 */
static float read_tacho(int file, struct sch5027_hwmon *s)
{
	unsigned int rpm;

	rpm = i2c_smbus_read_byte_data(file, s->offset);
	rpm |= i2c_smbus_read_byte_data(file, s->offset + 1) << 8;
	return ((rpm == 0xFFFF) ? 0 : ((s->scale == -1 ? 1 : s->scale) / rpm));
}

static float read_tacho_min(int file, struct sch5027_hwmon *s)
{
	unsigned int rpm;

	rpm = i2c_smbus_read_byte_data(file, s->min_offset);
	rpm |= i2c_smbus_read_byte_data(file, s->min_offset + 1) << 8;
	return ((rpm == 0xFFFF) ? 0 : ((s->scale == -1 ? 1 : s->scale) / rpm));
}

static void check_error(struct sch5027_hwmon *s)
{
	s->error = ((s->t_max == -1 ? 0 : s->cur > s->t_max) || \
			s->cur < s->t_min) ? 1 : 0;
}

int main(int argc, char *argv[])
{
	char *end;
	int res, i2cbus, address, size, file;
	int daddress;
	char filename[20];
	int pec = 0;
	int flags = 0;
	int force = 0, yes = 1, version = 0;
	int i, delay = 1, volt_func, count = 0;
	unsigned int gpio_base_addr;
	struct timeval tv;
	fd_set readfds;
	char c;

	/* handle (optional) flags first */
	while (1+flags < argc && argv[1+flags][0] == '-') {
		switch (argv[1+flags][1]) {
		case 'V': version = 1; break;
		case 'f': force = 1; break;
		case 'y': yes = 1; break;
		default:
			fprintf(stderr, "Error: Unsupported option "
				"\"%s\"!\n", argv[1+flags]);
			help();
			exit(1);
		}
		flags++;
	}

	if (version) {
		fprintf(stderr, "i2cget version %s\n", VERSION);
		exit(0);
	}

	if (argc < flags + 3)
		help();

	i2cbus = lookup_i2c_bus(argv[flags+1]);
	if (i2cbus < 0)
		help();

	address = parse_i2c_address(argv[flags+2]);
	if (address < 0)
		help();

	size = I2C_SMBUS_BYTE_DATA;
	struct sch5027_hwmon s[25] = {
		/* name, offset, min_offset, max_offset, t_min, t_max, min, max, cur, scale, volt_func */
		{"CPU", 0x25, 0x4E, 0x4F, -1, -1, -1, -1, -1, -1, -1},
		{"Internal Temp", 0x26, 0x50, 0x51, -1, -1, -1, -1, -1, -1, -1},
		{"Remote Diode2 Temp", 0x27, 0x52, 0x53, -1, -1, -1, -1, -1, -1, -1},
		{"+0.9V LAN1", 0x20, 0xFF, 0xFF, 0.855, 0.945, -1, -1, -1, 0.00586, 0},
		{"+0.9V LAN2", 0x20, 0xFF, 0xFF, 0.855, 0.945, -1, -1, -1, 0.00586, 1},
		{"+0.75V LAN2", 0x20, 0xFF, 0xFF, 0.675, 0.825, -1, -1, -1, 0.00586, 2},
		{"+12V", 0x20, 0xFF, 0xFF, 10.8, 13.2, -1, -1, -1, 0.07031, 3},
		{"CPU VCCIO", 0x21, 0xFF, 0xFF, 0.9, 1.1, -1, -1, -1, 0.01171, 0},
		{"+1.5V", 0x21, 0xFF, 0xFF, 1.35, 1.65, -1, -1, -1, 0.01171, 1},
		{"+1.5V DIMM", 0x21, 0xFF, 0xFF, 1.35, 1.65, -1, -1, -1, 0.01171, 2},
		{"CPU CORE", 0x21, 0xFF, 0xFF, 1.65, 1.86, -1, -1, -1, 0.01171, 3},
		{"+3.3V", 0x22, 0xFF, 0xFF, 2.97, 3.63, -1, -1, -1, 0.01718, -1},
		{"+2.5V FPGA", 0x23, 0xFF, 0xFF, 2.25, 2.75, -1, -1, -1, 0.02604, 0},
		{"+5V DUAL", 0x23, 0xFF, 0xFF, 4.5, 5.5, -1, -1, -1, 0.04375, 1},
		{"+5V SUB", 0x23, 0xFF, 0xFF, 4.5, 5.5, -1, -1, -1, 0.04375, 2},
		{"+5V", 0x23, 0xFF, 0xFF, 4.5, 5.5, -1, -1, -1, 0.04375, 3},
		{"+1.5V LAN1", 0x24, 0xFF, 0xFF, 1.425, 1.575, -1, -1, -1, 0.00781, 0},
		{"+1.5V LAN2", 0x24, 0xFF, 0xFF, 1.425, 1.575, -1, -1, -1, 0.00781, 1},
		{"+1.2V FPGA", 0x24, 0xFF, 0xFF, 1.08, 1.32, -1, -1, -1, 0.00586, 2},
		{"+1.05V", 0x24, 0xFF, 0xFF, 0.945, 1.155, -1, -1, -1, 0.00586, 3},
		{"+3.3VSB", 0x99, 0xFF, 0xFF, 2.97, 3.63, -1, -1, -1, 0.0172, -1},
		{"Rear Fan", 0x28, 0x54, -1, -1, -1, -1, -1, -1, 5400000, 4},
		{"Front Fan1", 0x2A, 0x56, -1, -1, -1, -1, -1, -1, 5400000, 4},
		{"Front Fan2", 0x2C, 0x58, -1, -1, -1, -1, -1, -1, 5400000, 4},
		{"Power Fan", 0x2E, 0x5A, -1, -1, -1, -1, -1, -1, 5400000, 4}
	};

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
	if (file < 0
	 || check_funcs(file, size, 0x00, pec)
	 || set_slave_addr(file, address, force))
		exit(1);

	gpio_base_addr = scan_pch_gpio_base();
	initscr();
	do {
		count++;
		/*
		 * In order to decrease the time of reading sensor, reading the sensor 
		 * in different for-loop.
		 */
		for (volt_func = 0; volt_func < 4; volt_func++) {
			/* 
			 *      HWM_DEV_EN  HWM_DEV_SEL  V2_IN        VCCP_IN            V1_IN        P5VTR_IN
			 * Def  (GPIO16)    (GPIO32)     (1.125V)     (2.25V)            (1.125V)     (5V)
			 *      0           0            V0R9_LAN1    V_CPU_VCCIO_RIGHT  V1R5_LAN1    V_2R5_FPGA
			 *      0           1            V0R9_LAN2    V_1R5_CORE         V1R5_LAN2    5VDUAL
			 *      1           0            V_0R75_VTT   V_1R5_DIMM         V_1R2_FPGA   V_5_SUB
			 *  ->  1           1            V_12_CORE    V_CPU_CORE         V_1R05_CORE  V_5_CORE
			 *
			 * S1031 use HWM_DEV_EN(GPIO16) and HWM_DEV_SEL(GPIO32) to contorl the 
			 * switch(TS3A5018).
			 */
			gpio_config(16, (volt_func >> 1) & 0x1, gpio_base_addr);
			gpio_config(32, volt_func & 0x1, gpio_base_addr);
			usleep(350000);

			for (i = 0; i < (sizeof(s) / sizeof(struct sch5027_hwmon)); i++) {
				if (s[i].volt_func == volt_func) {
					/* read via I2C */
					s[i].cur = read_sensor(file, &s[i]);
					if (s[i].cur < 0) {
						fprintf(stderr, "Error: Read failed\n");
						exit(2);
					}

					/* update min and max value */
					if (s[i].min == -1)
						s[i].min = s[i].cur;
					else
						s[i].min = min(s[i].cur, s[i].min);

					if (s[i].max == -1)
						s[i].max = s[i].cur;
					else
						s[i].max = max(s[i].cur, s[i].max);

					/* check error */
					check_error(&s[i]);
				}
			}
		}

		/* +3.3V, Temperature and Tachometer */
		for (i = 0; i < (sizeof(s) / sizeof(struct sch5027_hwmon)); i++) {
			if (s[i].volt_func == -1) {
				s[i].cur = read_sensor(file, &s[i]);
				if (s[i].cur < 0) {
					fprintf(stderr, "Error: Read failed\n");
					exit(2);
				}

				if (s[i].min_offset != 0xFF)
					s[i].t_min = read_sensor_min(file, &s[i]);

				if (s[i].max_offset != 0xFF)
					s[i].t_max = read_sensor_max(file, &s[i]);
			} else if (s[i].volt_func == 4) {
				s[i].cur = read_tacho(file, &s[i]);
				if (s[i].cur < 0) {
					fprintf(stderr, "Error: Read failed\n");
					exit(2);
				}

				if (s[i].min_offset != 0xFF)
					s[i].t_min = read_tacho_min(file, &s[i]);
			} else
				continue;

			/* update the min and max value */
			if (s[i].min == -1)
				s[i].min = s[i].cur;
			else
				s[i].min = min(s[i].cur, s[i].min);

			if (s[i].max == -1)
				s[i].max = s[i].cur;
			else
				s[i].max = max(s[i].cur, s[i].max);

			/* check error */
			check_error(&s[i]);

		}

		/*************************** display start ****************************/
		move(0, 0);

		if (has_colors() && start_color() == OK)
			init_pair(1, COLOR_WHITE, COLOR_RED);

		/* Temperature */
		attron(A_REVERSE);
		printw("%-20s %-10s %-10s %-10s %-10s %-10s\n", \
				"Temperature", "Current", "Min", "Max", "Min(Chk)", "Max(Chk)");
		attroff(A_REVERSE);
		for (i = 0; i < 3; i++) {
			if (s[i].error && has_colors() && start_color() == OK)
				attron(COLOR_PAIR(1));

			printw("%-20s %-10.1f %-10.1f %-10.1f %-10.1f %-10.1f\n", \
					s[i].name, s[i].cur, s[i].min, s[i].max, \
					s[i].t_min, s[i].t_max);
			if (s[i].error && has_colors() && start_color() == OK)
				attroff(COLOR_PAIR(1));
		}

		/* Voltage */
		attron(A_REVERSE);
		printw("%-20s %-10s %-10s %-10s %-10s %-10s\n", \
				"Voltage", "Current", "Min", "Max", "Min(Chk)", "Max(Chk)");
		attroff(A_REVERSE);
		for (i = 3; i < 21; i++) {
			if (s[i].error && has_colors() && start_color() == OK)
				attron(COLOR_PAIR(1));

			printw("%-20s %-10.3f %-10.3f %-10.3f %-10.3f %-10.3f\n", \
					s[i].name, s[i].cur, s[i].min, s[i].max, \
					s[i].t_min, s[i].t_max);
			if (s[i].error && has_colors() && start_color() == OK)
				attroff(COLOR_PAIR(1));
		}

		/* Tachometer */
		attron(A_REVERSE);
		printw("%-20s %-10s %-10s %-10s %-10s %-10s\n", \
				"Fan", "Current", "Min", "Max", "Min(Chk)", "Max(Chk)");
		attroff(A_REVERSE);
		for (i = 21; i < 25; i++) {
			if (s[i].error && has_colors() && start_color() == OK) {
				attron(COLOR_PAIR(1));
			}
			printw("%-20s %-10.0f %-10.0f %-10.0f %-10.0f %-10s\n", \
					s[i].name, s[i].cur, s[i].min, s[i].max, \
					s[i].t_min, "N/A");
			if (s[i].error && has_colors() && start_color() == OK) {
				attroff(COLOR_PAIR(1));
			}
		}

		/* footer */
		for (i = 0; i < sizeof(s) / sizeof(struct sch5027_hwmon); i++) {
			if (s[i].error)
				break;
		}
		
		attron(A_REVERSE);

		if (i != sizeof(s) / sizeof(struct sch5027_hwmon))
			printw("Something wrong, press any key to exit. %28s = %4d\n", "times", count);
		else
			printw("Press q or Ctrl+c to exit. %41s = %4d\n", "times", count);

		attroff(A_REVERSE);

		refresh();
		/*************************** display end ******************************/

		if (i != sizeof(s) / sizeof(struct sch5027_hwmon)) {
			noecho();
			getch();
			break;
		} else {
			FD_ZERO(&readfds);
			FD_SET(0, &readfds);
			tv.tv_sec = delay;
			tv.tv_usec = 0;
			if (select(1, &readfds, NULL, NULL, &tv) > 0) {
				if (read(0, &c, 1) != 1)
					break;
				if (c == 'q')
					delay = 0;
			}
		}
	} while (delay);

	endwin();
	close(file);

	return 0;
}
