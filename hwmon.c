#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#include <sitest.h>
#include <libsio.h>

/* Refer to AST 1300 firmware spec. ver.063 */
#define LOW_ADC_BASE_ADDR    0x1600
#define HIGH_ADC_BASE_ADDR   0x1E72

#define LOW_DATA_BASE_ADDR   0x2000
#define HIGH_DATA_BASE_ADDR  0x1E72

#define LOW_I2C_BASE_ADDR    0x1700
#define HIGH_I2C_BASE_ADDR   0x1E72

#define LOW_TACHO_BASE_ADDR  0x1500
#define HIGH_TACHO_BASE_ADDR 0x1E72

#define LOW_PECI_BASE_ADDR   0x1C00
#define HIGH_PECI_BASE_ADDR  0x1E72

/* The base address is 0x1E720000 */
#define HIGH_CTL_BASE_ADDR   0x1E72

/* 
 * Define the struct for Sensor.
 * Type 0: Temperature; 1: FAN Speed; 2: Voltage > 0 and < 2.048V; 
 * 3: Voltage >2.048V
 * Name: Sensor Name
 * Index: The index of the register in Super IO
 * Bank: The bank of the register in Super IO
 * Par1 and Par2: For Fan speed, Par1 is a value when no fan installed, 
 * the program will set the speed to 0 if the speed is Par1.
 * For Voltage (Type 3), Par1 is R1, Par2 is R2
 * Low and Up: The range of the sensors, if over range, 
 * the program will show Over range.
 */
struct sensor {
	int type;
	char name[30];
	char pin_name[5];
	unsigned int index;
	int bank;
	float par1;
	float par2;
	float low_limit;
	float high_limit;
	float min;
	float max;
	float multiplier;
};

void bank_select(unsigned int address, unsigned int bank);
void init_peci(void);

unsigned int read_hwmon_base_address(void);

float read_temperature(unsigned int address, int plus, struct sensor *sensors);
float read_fan_speed(unsigned int address, int plus, struct sensor *sensors);
float read_voltage1(unsigned int address, int plus, struct sensor *sensors);
float read_voltage2(unsigned int address, int plus, struct sensor *sensors);
float read_voltage3(unsigned int address, int plus, struct sensor *sensors);

float read_ast_temperature_peci(unsigned int index, ...);
float read_ast_temperature_i2c(unsigned int index, ...);
float read_ast_fan(unsigned int index, ...);
float read_ast_voltage(unsigned int index, ...);
float read_ast_voltage1(unsigned int index, ...);

void usage(void);

int type_list(struct sensor *sensors);
int ast_type_list(struct sensor *sensors);
void pin_list(char *chip_model, struct sensor *sensors);

char chip_model[10];

int main(int argc, unsigned char *argv[])
{
	FILE *fp;
	char *buf;
	unsigned int hw_base_addr, offset;
	int i = 0, j = 0, count = 0, result = 0, time, plus;
	float b = 0;

	/* read sensor functions initialize, for SIO */
	float (*read_sio_sensor[])(unsigned int address, int plus , \
                               struct sensor *sensors) = {
		read_temperature,
		read_fan_speed,
		read_voltage1,
		read_voltage2,
		read_voltage3
	};

	/* read sensor functions initialize, for AST1300 */
	float (*read_ast_sensor[])(unsigned int index, ...) = {
		read_ast_temperature_peci,
		read_ast_temperature_i2c,
		read_ast_fan,
		read_ast_voltage,
		read_ast_voltage1
	};

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}
	
	/* If the parameter is wrong, use the usage message */
	if (argc != 3) {
		usage();
		exit(-1);
	}

	time = atoi(argv[2]);

	fp = fopen(argv[1], "r");

	if ( fp == NULL) {
		ERR("Fail to open <%s>, please enter the correct file name.\n", argv[1]);
		exit(-1);
	}

	/* Skip the header */
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);
	fscanf(fp, "%*[^\n]\n", NULL);
	/* Remember the offset, read from this position later. */
	offset = ftell(fp);
	DBG("offset = %d\n", offset);
	/* Skip 4th line. */
	fscanf(fp, "%*[^\n]\n", NULL);

	/* Count how many data we need to read. */
	while (!feof(fp)) {
		fscanf(fp, "%*[^\n]\n", NULL);
		count++;
	}
	DBG("count = %d\n", count);

	fseek(fp, offset, SEEK_SET);

	struct sensor sensors[count];

	fscanf(fp, "%[^,], %x\n", chip_model, &EFER);
	EFDR = EFER + 1;
	DBG("chip_model = %s, EFER = %x\n", chip_model, EFER);

	if (strncmp("AST", chip_model, 3) == 0) {
		while (!feof(fp)) {
			fscanf(fp, "%[^,], %[^,], %f, %f, %f, %f, %f\n", \
                        &sensors[j].name, &sensors[j].pin_name, \
                        &sensors[j].par1, &sensors[j].par2, \
                        &sensors[j].low_limit, &sensors[j].high_limit, \
                        &sensors[j].multiplier);

			DBG("name = %s, pin_name = %s, par1 = %f, par2 = %f, " \
                "low_limit = %f, high_limit = %f, multiplier = %f\n", \
                 sensors[j].name, sensors[j].pin_name, sensors[j].par1, \
                 sensors[j].par2, sensors[j].low_limit, sensors[j].high_limit, \
                 sensors[j].multiplier);

			pin_list(chip_model, &sensors[j]);
			sensors[j].type = ast_type_list(&sensors[j]);
			DBG("sensors[%d].type = %d\n", j, sensors[j].type);

			DBG("name = %s, type = %d, index = %x, par1 = %f, par2 = %f, " \
                "low_limit = %f, high_limit = %f, multiplier = %f\n", \
                 sensors[j].name, sensors[j].type, sensors[j].index, \
                 sensors[j].par1, sensors[j].par2, sensors[j].low_limit, \
                 sensors[j].high_limit, sensors[j].multiplier);

			j++;
		}
	} else {
		while (!feof(fp)) {
			fscanf(fp, "%[^,], %d, %f, %f, %f, %f, %f\n", \
                        &sensors[j].name, &sensors[j].index, &sensors[j].par1, \
                        &sensors[j].par2, &sensors[j].low_limit, \
                        &sensors[j].high_limit, &sensors[j].multiplier);

			DBG("name = %s, par1 = %f, par2 = %f, low_limit = %f, " \
                "high_limit = %f, multiplier = %f\n", \
                 sensors[j].name, sensors[j].par1, sensors[j].par2, \
                 sensors[j].low_limit, sensors[j].high_limit, \
                 sensors[j].multiplier);

			pin_list(chip_model, &sensors[j]);
			sensors[j].type = type_list(&sensors[j]);

			DBG("name = %s, type = %d, index = %x, par1 = %f, par2 = %f, " \
                "low_limit = %f, high_limit = %f, multiplier = %f\n", \
                 sensors[j].name, sensors[j].type, sensors[j].index, \
                 sensors[j].par1, sensors[j].par2, sensors[j].low_limit, \
                 sensors[j].high_limit, sensors[j].multiplier);

			j++;
		}
	}

	fclose(fp);

	/* Enter the Extended Function Mode */
	sio_enter(chip_model);
	if (strncmp("AST", chip_model, 3) == 0) {
		/* 
		 * Always access AST1300 with SuperIO protocol, so do not exit 
		 * Entended Function Mode.
		 */
		init_peci();
	} else {
		/* Read HW monitor base address */
		hw_base_addr = read_hwmon_base_address();
		sio_exit();
	}

	if (strncmp("NCT", chip_model, 3) == 0 || \
        strncmp("W83", chip_model, 3) == 0) {
		plus = 5;
	} else if (strncmp("F71889", chip_model, 6) == 0) {
		plus = 0;
	} else if (strncmp("F7186", chip_model, 5) == 0) {
		plus = 5;
	}

	/* Disable the buffer of STDOUT */
	setbuf(stdout, NULL);

	/* The loop for Test time */
	for (i = 0; i < time; i++) {

#ifndef DEBUG
		system("clear");
		printf("Sensors                       Current     Minimum     Maximum     Status\n");
		printf("--------------------------------------------------------------------------");
#endif

		/* 
		 * Read the sensors and show the current value and record the min/max 
         * value.
		 */
		for (j = 0; j < count; j++) {
			if (strncmp("AST", chip_model, 3) == 0) {
				DBG("j = %d, type = %d, index = %x, par1 = %f, par2 = %f\n",
                     j, sensors[j].type, sensors[j].index, \
                     sensors[j].par1, sensors[j].par2);

				b = read_ast_sensor[sensors[j].type](sensors[j].index, \
                                                     sensors[j].par1, \
                                                     sensors[j].multiplier);
			
				DBG("b = %f\n", b);
			} else {
				DBG("hw_base_addr = %x, j = %d, type = %d, index = %x, " \
                    "bank = %d, par1 = %f, par2 = %f\n", 
                     hw_base_addr, j, sensors[j].type, sensors[j].index, \
                     sensors[j].bank, sensors[j].par1, sensors[j].par2);

				if (strncmp("NCT", chip_model, 3) == 0 || \
                    strncmp("W83", chip_model, 3) == 0) {
					bank_select(hw_base_addr, sensors[j].bank); /* Set Bank */
				}

				b = read_sio_sensor[sensors[j].type](hw_base_addr, plus, \
                                                     &sensors[j]);
				DBG("b = %f\n", b);
			}

#ifndef DEBUG
			/* Show Sensor name */
			asprintf(&buf, "tput cup %d 0", j + 2);
			system(buf);
			free(buf);
			printf("%s", sensors[j].name);

			/* Show Current value */
			asprintf(&buf, "tput cup %d 30", j + 2);
			system(buf);
			free(buf);
			printf("%.3f", b);

			/* Initial the Min and Max value at the first time */
			if (i == 0) {
				sensors[j].max = b;
				sensors[j].min = b;
			} else { /* Change the Min and Max value if the current value is */
				if (b > sensors[j].max) 
					sensors[j].max = b;

				if (b < sensors[j].min) 
					sensors[j].min = b;
			}

			/* Show minimum value */
			asprintf(&buf, "tput cup %d 42", j + 2);
			system(buf);
			free(buf);
			printf("%.3f", sensors[j].min);

			/* Show maximum value */
			asprintf(&buf, "tput cup %d 54", j + 2);
			system(buf);
			free(buf);
			printf("%.3f", sensors[j].max);

			/* Show Status */
			asprintf(&buf, "tput cup %d 66", j + 2);
			system(buf);
			free(buf);

			if ((sensors[j].min >= sensors[j].low_limit) && \
                (sensors[j].max <= sensors[j].high_limit)) {
				printf("PASS");
			} else {
				printf("Fail!!");
				result = 1;
			}
#endif
		}

		sleep(1);
	}

	if (strncmp("AST", chip_model, 3) == 0)
		sio_exit();

	printf("\n\n");

	return result;
}

void init_peci(void)
{
	unsigned int data;

	/* Enable PECI, Negotiation Timing = 0x40, Clock divider = 2 */
	sio_ilpc2ahb_write(0x02, LOW_PECI_BASE_ADDR + 0x00, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x40, LOW_PECI_BASE_ADDR + 0x01, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x02, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x03, HIGH_PECI_BASE_ADDR);

	/* Read length = 2, Write length = 1, CPU address = 0x30 */
	sio_ilpc2ahb_write(0x30, LOW_PECI_BASE_ADDR + 0x04, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x05, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x02, LOW_PECI_BASE_ADDR + 0x06, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x07, HIGH_PECI_BASE_ADDR);

	/* Write Command Code (0x01) into write Register */
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x0C, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x0D, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x0E, HIGH_PECI_BASE_ADDR);
	sio_ilpc2ahb_write(0x00, LOW_PECI_BASE_ADDR + 0x0F, HIGH_PECI_BASE_ADDR);

	/* Fire Engine */
	sio_ilpc2ahb_write(0x01, LOW_PECI_BASE_ADDR + 0x08, HIGH_PECI_BASE_ADDR);

	/* Check the status is Idle or Busy */
	data = 2;
	while (data != 0) {
		usleep(10000);
		data = sio_ilpc2ahb_read(LOW_PECI_BASE_ADDR + 0x08, HIGH_PECI_BASE_ADDR);
		data &= 0x02;
	}
}

/* 
 * Read HW monitor base address 
 * The base address of the Address Port and Data Port is specified in registers 
 * CR[60h] and CR[61h] of Logical Device B, the hardware monitor device. 
 * CR[60h] is the high byte, and CR[61h] is the low byte. The Address Port
 * and Data Port are located at the base address, plus 5h and 6h, respectively. 
 * For example, if CR[60h] is 02h and CR[61h] is 90h, the Address Port is at 
 * 0x295h, and the Data Port is at 0x296h
 */
unsigned int read_hwmon_base_address(void)
{
	unsigned int addr, high_addr, low_addr;

	if (strncmp("NCT", chip_model, 3) == 0 || \
        strncmp("W83", chip_model, 3) == 0) {
		sio_select(0x0B); /* Select Logical device B */
	} else { /* For Fintek */
		sio_select(0x04); /* Logical device number 4 */
	}

	sio_logical_device_enable(SIO_HWMON_EN);
	
	/* Read the value of CR 60h of Logical device */
	high_addr = (sio_read(SIO_HW_BASE_REG) << 8);
	/* Read the value of CR 61h of Logical device */
	low_addr = sio_read(SIO_HW_BASE_REG + 1);
	addr = high_addr | low_addr;
	DBG("high_addr = %x, low_addr = %x, addr = %x\n", \
        high_addr, low_addr, addr);

	return addr;
}

void bank_select(unsigned int address, unsigned int bank)
{
	unsigned char data;
	int plus = 5;

	outb_p(EFER, address + plus); /* Bank select */
	data = inb_p(address + plus + 1);

	if (strcmp("NCT6776F", chip_model) == 0 || \
        strncmp("W83", chip_model, 3) == 0) {
		data &= ~(0x7); /* Clear bit0~2 */
	} else if (strcmp("NCT6779D", chip_model) == 0) {
		data &= ~(0x0F); /* Clear bit0~3 */
	}

	data |= (bank); /* Set Bank */
	outb_p(data, address + plus + 1);
}

int type_list(struct sensor *sensors)
{
	if (sensors->multiplier != 0) {
		return 4;
	} else if (sensors->par2 == 0) {
		if (sensors->high_limit < 20) {
			return 2;
		} else if (sensors->high_limit < 110 && sensors->high_limit >= 20) {
			return 0;
		} else {
			return 1;
		}
	} else {
		return 3;
	}
}

int ast_type_list(struct sensor *sensors)
{
	return (sensors->multiplier != 0 ? 4 : sensors->type);
}

float read_temperature(unsigned int address, int plus, struct sensor *sensors)
{
	return (sio_read_reg(sensors->index, address + plus) + sensors->par1);
}

float read_fan_speed(unsigned int address, int plus, struct sensor *sensors)
{
	int data;

	data = sio_read_reg(sensors->index, address + plus);
	DBG("data = %d\n", data);
	data = data << 8;
	DBG("data = %d\n", data);
	data |= sio_read_reg(sensors->index + 1, address + plus);
	DBG("data = %d\n", data);

	if (strncmp("F718", chip_model, 4) == 0) {
		data = 1500000 / data;
	}

#if 0
	if (data == sensors->par1) /* If the fan speed = par1, set the fan speed =0 */
		data = 0;
#endif

	return (data == sensors->par1) ? 0 : data;
}

float read_voltage1(unsigned int address, int plus, struct sensor *sensors)
{
	return ((float) sio_read_reg(sensors->index, address + plus) * 0.008);
}

float read_voltage2(unsigned int address, int plus, struct sensor *sensors)
{
	int data;
	float result;

	data = sio_read_reg(sensors->index, address + plus);

	DBG("address = %x, index = %x, par1 = %f, par2 = %f, data = %d\n", \
         address, sensors->index, sensors->par1, sensors->par2, data);

	result = ((float) data) * ((float) (sensors->par1 + sensors->par2)) / \
             ((float) sensors->par2) * 0.008;
	
	return result;
}

float read_voltage3(unsigned int address, int plus, struct sensor *sensors)
{
	DBG("multiplier = %f\n", sensors->multiplier);
	return ((float) sio_read_reg(sensors->index, address + plus) * \
            sensors->multiplier / 1000.0);
}

float read_ast_temperature_peci(unsigned int index, ...)
{
	unsigned int data_l = 0;
	unsigned int data_h = 0;
	unsigned int data = 0;
	int low_addr;
	int high_addr;

	low_addr = LOW_DATA_BASE_ADDR + index;
	high_addr = HIGH_DATA_BASE_ADDR;

	DBG("high_addr = %x, low_addr = %x\n", high_addr, low_addr);

	data_l = sio_ilpc2ahb_read(low_addr, high_addr);
	data_h = sio_ilpc2ahb_read(low_addr + 1, high_addr);
	data = data_l | (data_h << 8);

	return ((float) data);
}

float read_ast_temperature_i2c(unsigned int index, ...)
{
	unsigned int data_l = 0;
	unsigned int data_h = 0;
	unsigned int data = 0;
	int low_addr;
	int high_addr;
	float result;
	int Hz = 100000;

	low_addr = LOW_I2C_BASE_ADDR;
	high_addr = HIGH_I2C_BASE_ADDR;

	/* Write I2C Channel 7 clock to 100KHz */
	sio_ilpc2ahb_write((Hz & 0xFF), low_addr + 0x48, high_addr);
	sio_ilpc2ahb_write(((Hz >> 8) & 0xFF), low_addr + 0x49, high_addr);
	sio_ilpc2ahb_write(((Hz >> 16) & 0xFF), low_addr + 0x4A, high_addr);
	sio_ilpc2ahb_write(((Hz >> 24) & 0xFF), low_addr + 0x4B, high_addr);

	/* Set I2C to read, Device Address and Channel number */
	sio_ilpc2ahb_write(0x07, low_addr + 0x54, high_addr);
	sio_ilpc2ahb_write(index, low_addr + 0x55, high_addr);
	sio_ilpc2ahb_write(0x00, low_addr + 0x56, high_addr);
	sio_ilpc2ahb_write(0x00, low_addr + 0x57, high_addr);
		
	/* Enabled I2C Channel 7 */
	data = sio_ilpc2ahb_read(low_addr + 0x00, high_addr);
	data |= 0x40;
	sio_ilpc2ahb_write(data, low_addr + 0x00, high_addr);

	/* Fire to Engine */
	data = sio_ilpc2ahb_read(low_addr + 0x5C, high_addr);
	data |= 0x01;
	sio_ilpc2ahb_write(data, low_addr + 0x5C, high_addr);

	usleep(1000000); /* Need the wait time for next reading */

	data_l = 2; /* Check the status is Idle or Busy */
	while (data_l != 0) {
		usleep(10000);
		data_l = sio_ilpc2ahb_read(low_addr + 0x5C, high_addr);
		data_l &= 0x02;
	}

	/* Read I2C data */
	result = (sio_ilpc2ahb_read(LOW_DATA_BASE_ADDR, HIGH_DATA_BASE_ADDR));

	return result;
}

float read_ast_fan(unsigned int index, ...)
{
	unsigned int data_l = 0;
	unsigned int data_h = 0;
	unsigned int data = 0;
	int low_addr;
	int high_addr;

	low_addr = LOW_TACHO_BASE_ADDR + index;
	high_addr = HIGH_TACHO_BASE_ADDR;

	data_l = sio_ilpc2ahb_read(low_addr, high_addr);
	data_h = sio_ilpc2ahb_read(low_addr + 1, high_addr);
	data = data_l | (data_h << 8);

	return ((float) data);
}

float read_ast_voltage(unsigned int index, ...)
{
	unsigned int data_l = 0;
	unsigned int data_h = 0;
	unsigned int data = 0;
	int low_addr;
	int high_addr;
	float par1, result;
	va_list args;

	low_addr = LOW_ADC_BASE_ADDR + index;
	high_addr = HIGH_ADC_BASE_ADDR;
	data_l = sio_ilpc2ahb_read(low_addr, high_addr);
	data_h = sio_ilpc2ahb_read(low_addr + 1, high_addr);
	data_h &= 0x03;
	data = data_l | (data_h << 8);

	va_start(args, index);
	par1 = va_arg(args, double);
	va_end(args);

	/* Ask BIOS to get the ratio */
	result = (((float) data) * ((float) par1) * 25 / 1023 / 100);

	return result;
}

float read_ast_voltage1(unsigned int index, ...)
{
	unsigned int data_l = 0;
	unsigned int data_h = 0;
	unsigned int data = 0;
	int low_addr;
	int high_addr;
	float multiplier;
	va_list args;

	low_addr = LOW_ADC_BASE_ADDR + index;
	high_addr = HIGH_ADC_BASE_ADDR;
	data_l = sio_ilpc2ahb_read(low_addr, high_addr);
	data_h = sio_ilpc2ahb_read(low_addr + 1, high_addr);
	data_h &= 0x03;
	data = data_l | (data_h << 8);

	va_start(args, index);
	multiplier = va_arg(args, double);
	multiplier = va_arg(args, double);
	va_end(args);

	/* Ask BIOS to get the ratio */
	return (((float) data) * multiplier / 1024);
}


/* TODO: need to find a better way to do this... */
void pin_list(char *chip_model, struct sensor *sensors)
{
	int pin = sensors->index;

	if (strcmp("NCT6776D", chip_model) == 0 || strcmp("NCT6776F", chip_model) == 0) {
		switch(pin) {
			case 1:
				sensors->index = 0x23;
				sensors->bank = 0;
				break;
			case 46:
				sensors->index = 0x50;
				sensors->bank = 5;
				break;
			case 99:
				sensors->index = 0x51;
				sensors->bank = 5;
				break;
			case 103:
				sensors->index = 0x25;
				sensors->bank = 0;
				break;
			case 104:
				sensors->index = 0x24;
				sensors->bank = 0;
				break;
			case 105:
				sensors->index = 0x21;
				sensors->bank = 0;
				break;
			case 106:
				sensors->index = 0x22;
				sensors->bank = 0;
				break;
			case 107:
				sensors->index = 0x20;
				sensors->bank = 0;
				break;
			case 109:
				sensors->index = 0x26;
				sensors->bank = 0;
				break;
			case 124:
				sensors->index = 0x58;
				sensors->bank = 6;
				break;
			case 126:
				sensors->index = 0x56;
				sensors->bank = 6;
				break;
			case 110:
				sensors->index = 0x50;
				sensors->bank = 1;
				break;
			case 111:
				sensors->index = 0x2B;
				sensors->bank = 6;
				break;
		}
	} else if (strcmp("NCT6779D", chip_model) == 0) {
		switch(pin) {
			case 3:
				sensors->index = 0xC4;
				sensors->bank = 4;
				break;
			case 4:
				sensors->index = 0xC6;
				sensors->bank = 4;
				break;
			case 99:
				sensors->index = 0x88;
				sensors->bank = 4;
				break;
			case 104:
				sensors->index = 0x84;
				sensors->bank = 4;
				break;
			case 105:
				sensors->index = 0x81;
				sensors->bank = 4;
				break;
			case 106:
				sensors->index = 0x8C;
				sensors->bank = 4;
				break;
			case 107:
				sensors->index = 0x8D;
				sensors->bank = 4;
				break;
			case 109:
				sensors->index = 0x80;
				sensors->bank = 4;
				break;
			case 111:
				sensors->index = 0x86;
				sensors->bank = 4;
				break;
			case 112:
				sensors->index = 0x20;
				sensors->bank = 7;
				break;
			case 113:
				sensors->index = 0x90;
				sensors->bank = 4;
				break;
			case 114:
				sensors->index = 0x8A;
				sensors->bank = 4;
				break;
			case 115:
				sensors->index = 0x8B;
				sensors->bank = 4;
				break;
			case 116:
				sensors->index = 0x8E;
				sensors->bank = 4;
				break;
			case 124:
				sensors->index = 0xC2;
				sensors->bank = 4;
				break;
			case 126:
				sensors->index = 0xC0;
				sensors->bank = 4;
				break;
		}
	} else if (strncmp("W83", chip_model, 3) == 0) {
		switch (pin) {
			case 28:
				sensors->index = 0x23;
				sensors->bank = 0;
				break;
			case 61:
				sensors->index = 0x50;
				sensors->bank = 5;
				break;
			case 74:
				sensors->index = 0x51;
				sensors->bank = 5;
				break;
			case 95:
				sensors->index = 0x22;
				sensors->bank = 0;
				break;
			case 96:
				sensors->index = 0x26;
				sensors->bank = 0;
				break;
			case 97: 
				sensors->index = 0x25;
				sensors->bank = 0;
				break;
			case 99:
				sensors->index = 0x21;
				sensors->bank = 0;
				break;
			case 100:
				sensors->index = 0x20;
				sensors->bank = 0;
				break;
			case 102:
				sensors->index = 0x50;
				sensors->bank = 2;
				break;
			case 103:
				sensors->index = 0x50;
				sensors->bank = 1;
				break;
			case 104:
				sensors->index = 0x27;
				sensors->bank = 0;
				break;
			case 111:
				sensors->index = 0x54;
				sensors->bank = 6;
				break;
			case 112:
				sensors->index = 0x50;
				sensors->bank = 6;
				break;
			case 119:
				sensors->index = 0x52;
				sensors->bank = 6;
				break;
		}
	} else if (strncmp("F718", chip_model, 4) == 0) {
		sensors->bank = 0;
		switch (pin) {
			case 21:
				sensors->index = 0xA0;
				break;
			case 23:
				sensors->index = 0xB0;
				break;
			case 25:
				sensors->index = 0xC0;
				break;
			case 89:
				sensors->index = 0x76;
				break;
			case 90:
				sensors->index = 0x74;
				break;
			case 91:
				sensors->index = 0x72;
				break;
			case 93:
				sensors->index = 0x26;
				break;
			case 94:
				sensors->index = 0x25;
				break;
			case 95:
				sensors->index = 0x24;
				break;
			case 96:
				sensors->index = 0x23;
				break;
			case 97:
				sensors->index = 0x22;
				break;
			case 98:
				sensors->index = 0x21;
				break;
		}

		if (strncmp("F7186", chip_model, 5)  ==  0) {
			switch(pin) {
				case 4:
				case 37:
					sensors->index = 0x20;
					break;
				case 58:
					sensors->index = 0x7E;
					break;
				case 68:
					sensors->index = 0x27;
					break;
				case 86:
					sensors->index = 0x28;
					break;
			}
		}

		if (strncmp("F71889", chip_model, 5)  ==  0) {
			switch(pin) {
				case 1:
				case 35:
					sensors->index = 0x20;
					break;
				case 44:
					sensors->index = 0x78;
					break;
				case 65:
					sensors->index = 0x27;
					break;
				case 82:
					sensors->index = 0x28;
					break;
			}
		}
	} else if (strcmp("AST1300", chip_model)  ==  0) {
		if (strcmp(sensors->pin_name, "AA21") == 0) { /* PECI */
			if (strcmp(sensors->name, "TEMP_CPU0") == 0)
				sensors->index = 0x50;
			else if (strcmp(sensors->name, "TEMP_CPU1") == 0)
				sensors->index = 0x54;
			else if (strcmp(sensors->name, "TEMP_CPU0_VR") == 0) {
				sensors->index = 0x5C;
			} else {
				ERR("Sensor name should be TEMP_CPU0 or TEMP_CPU1 or TEMP_CPU0_VR\n");
				exit(-1);
			}
			sensors->type = 0;
		} else if (strcmp(sensors->pin_name, "D1") == 0) { /* I2C */
			sensors->type = 1;
			if (strcmp(sensors->name, "TEMP_BMC") == 0)
				sensors->index = 0x90; /* device addres 0x90 */
			else if (strcmp(sensors->name, "TEMP_ENV") == 0)
				sensors->index = 0x98; /* device addres 0x98 */
			else {
				ERR("Sensor name should be TEMP_BMC or TEMP_ENV\n");
				exit(-1);
			}
		/* Tachometer, Channel 0 ~ 15 */
		} else if (strcmp(sensors->pin_name, "V6") == 0) {
			sensors->index = 0x08;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "Y5") == 0) {
			sensors->index = 0x0C;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AA4") == 0) {
			sensors->index = 0x10;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AB3") == 0) {
			sensors->index = 0x14;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "W6") == 0) {
			sensors->index = 0x18;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AA5") == 0) {
			sensors->index = 0x1C;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AB4") == 0) {
			sensors->index = 0x20;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "V7") == 0) {
			sensors->index = 0x24;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "Y6") == 0) {
			sensors->index = 0x28;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AB5") == 0) {
			sensors->index = 0x2C;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "W7") == 0) {
			sensors->index = 0x30;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AA6") == 0) {
			sensors->index = 0x34;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AB6") == 0) {
			sensors->index = 0x38;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "Y7") == 0) {
			sensors->index = 0x3C;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AA7") == 0) {
			sensors->index = 0x40;
			sensors->type = 2;
		} else if (strcmp(sensors->pin_name, "AB7") == 0) {
			sensors->index = 0x44;
			sensors->type = 2;
		}
		/* ADC0 ~ ADC11 */
		else if (strcmp(sensors->pin_name, "L5") == 0) {
			sensors->index = 0x08;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "L4") == 0) {
			sensors->index = 0x0C;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "L3") == 0) {
			sensors->index = 0x10;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "L2") == 0) {
			sensors->index = 0x14;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "L1") == 0) {
			sensors->index = 0x18;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "M5") == 0) {
			sensors->index = 0x1C;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "M4") == 0) {
			sensors->index = 0x20;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "M3") == 0) {
			sensors->index = 0x24;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "M2") == 0) {
			sensors->index = 0x28;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "M1") == 0) {
			sensors->index = 0x2C;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "N5") == 0) {
			sensors->index = 0x30;
			sensors->type = 3;
		} else if (strcmp(sensors->pin_name, "N4") == 0) {
			sensors->index = 0x34;
			sensors->type = 3;
		}
	}
}

/* If parameter is wrong, it will show this message */
void usage(void)
{
	printf("Usage : hwmon <FILE NAME> <TIMES>\n"
           " i.e: ./hwmon S0361.conf 10\n");
}
