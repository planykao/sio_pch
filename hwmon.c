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
#include <siolib.h>
#include <pin_list.h>

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

void bank_select(unsigned int address, unsigned int bank);
void Type_list(unsigned int *type, float par1, float par2, float up);
void init_PECI(void);
void write_reg(unsigned char val_w, unsigned int lw, unsigned int hw);

unsigned int read_reg(unsigned int lr, unsigned int hr);
unsigned int read_hwmon_base_address(void);
unsigned char read_sio(unsigned int index);

float read_sio_sensor(unsigned int address, unsigned int type, \
                      unsigned int index, unsigned int bank, \
                      float par1, float par2);
float read_ast_sensor(unsigned int type, unsigned int index, \
                      float par1, float par2);

float read_temperature(unsigned int address, unsigned int index, int plus, float par1, float par2);
float read_fan_speed(unsigned int address, unsigned int index, int plus, float par1, float par2);
float read_voltage1(unsigned int address, unsigned int index, int plus, float par1, float par2);
float read_voltage2(unsigned int address, unsigned int index, int plus, float par1, float par2);
float read_ast_temperature();
float read_ast_fan_speed();
float read_ast_voltage1();
float read_ast_voltage2();

void usage(void);

char chip_model[10];

/* Define the struct for Sensor.
 * Type 0: Temperature; 1: FAN Speed; 2: Voltage > 0 and < 2.048V; 
 * 3: Voltage >2.048V
 * Name: Sensor Name
 * Index: The index of the register in Super IO
 * Bank: The bank of the register in Super IO
 * Par1 and Par2: For Fan speed, Par1 is a value when no fan installed, 
 * the program will set the speed to 0 if the speed is Par1.
 * For Voltage (Type 3), Par1 is R1, Par2 is R2
 * Low and Up: The range of the sensors, if over range, 
 * the program will show Over range. */
struct sensor {
	unsigned int type;
	unsigned char name[30];
	unsigned int index;
	unsigned int bank;
	float par1;
	float par2;
	float low;
	float up;
	float min;
	float max;
};

int main(int argc, unsigned char *argv[])
{
	FILE *fp;
	char ch;
	char *buf;
	unsigned int hw_base_addr;
	int i = 0, j = 0, count = 0;
	int Result = 0, Size = 0, line_str = 2;
	float b = 0;
	unsigned char Time;

	/* read sensor functions initialize */
	float (*read_sio_sensor1[])(unsigned int address, unsigned index, int plus, float par1, float par2) = {
		read_temperature,
		read_fan_speed,
		read_voltage1,
		read_voltage2
	};

	float (*read_ast_sensor1[])() = {
		read_ast_temperature,
		read_ast_fan_speed,
		read_ast_voltage1,
		read_ast_voltage2
	};

	/* change I/O privilege level to all access. For Linux only. */
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}
	
	/* If the parameter is wrong, use the usage message */
	if (argc != 2) {
		usage();
		exit(-1);
	}

	Time = atoi(argv[1]);

	fp=fopen("hwm.conf", "r");

	while ((ch = fgetc(fp)) != EOF) {
		if (count == 0) {
			chip_model[i] = ch;
			i = i + 1;
		}

		if (ch == '\n')
			++count;
	}
	chip_model[i - 1] = '\0';

	rewind(fp);

	count = count - line_str;
	struct sensor sensors[count];

	fscanf(fp, "%*[^\n]\n");
	while (!feof(fp)) {
		/* Sensors Type, Name, Index, Bank number, Low limition, Up limition */
		fscanf(fp, "%[^,], %d, %f, %f, %f, %f\n", \
				&sensors[j].name, &sensors[j].index, &sensors[j].par1, \
				&sensors[j].par2, &sensors[j].low, &sensors[j].up);

		DBG("sensors[%d].par2 = %d\n", j, sensors[j].par2);
		j = j + 1;
	}
	fclose(fp);

	for (j = 0; j < count; j++) {
		Pin_list(chip_model, &sensors[j].index, &sensors[j].type, \
                 &sensors[j].bank);
		Type_list(&sensors[j].type, sensors[j].par1, sensors[j].par2, \
                  sensors[j].up);
	}

	if (strncmp("AST", chip_model, 3) == 0) {
		EFER = 0x2E;
		EFDR = 0x2F;
		sio_enter(chip_model);
		init_PECI();
	} else {
		EFER = 0x4E;
		EFDR = 0x4F;
		sio_enter(chip_model);
		/* Read HW monitor base address */
		hw_base_addr = read_hwmon_base_address();
	}

	sio_exit();

	/* Disable the buffer of STDOUT */
	setbuf(stdout, NULL);

	/* The loop for Test time */
	for (i = 0; i < Time; i++) {
		system("clear");
		printf("Sensors                       Current     Minimum     Maximum     Status\n");
		printf("--------------------------------------------------------------------------");

		/* Read the sensors and show the current value and record the min/max value. */
		for (j = 0; j < count; j++) {
			if (strncmp("AST", chip_model, 3) == 0) {
				b = read_ast_sensor(sensors[i].type, sensors[i].index, \
                                    sensors[i].par1, sensors[i].par2);
			} else {
				DBG("\nhw_base_addr = %x, j = %d, type = %d, index = %d,\
						bank = %d, par1 = %f, par2 = %f\n", hw_base_addr, j, \
						sensors[j].type, sensors[j].index, sensors[j].bank, \
						sensors[j].par1, sensors[j].par2);
#if 0
				b = read_sio_sensor(hw_base_addr, sensors[j].type, \
                                    sensors[j].index, sensors[j].bank, \
                                    sensors[j].par1, sensors[j].par2);
#else
				if (strncmp("NCT", chip_model, 3) == 0) {
					bank_select(hw_base_addr, sensors[j].bank); /* Set Bank */
				}

				int plus;

				if (strncmp("NCT", chip_model, 3) == 0) {
					plus = 5;
				} else if (strcmp("F71889AD", chip_model) == 0) {
					plus = 0;
				} else if (strncmp("F7186", chip_model, 5) == 0) {
					plus = 5;
				}

				b = read_sio_sensor1[sensors[j].type](hw_base_addr, \
                                                      sensors[j].index, \
													  plus, \
                                                      sensors[j].par1, \
                                                      sensors[j].par2);
				DBG("b = %f\n", b);
#endif
			}
		
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
			} else {  /* Change the Min and Max value if the current value is */
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

			if ((sensors[j].min >= sensors[j].low) && \
                (sensors[j].max <= sensors[j].up)) {
				printf("PASS");
			} else {
				printf("Fail!!");
				Result = 1;
			}
		}

		sleep(1);
	}

	printf("\n\n");

	return Result;
}

#if 0
unsigned char read_sio(unsigned int index)
{
	unsigned char b;

	outb_p(index, EFER); /* Sent index to EFER */
	b = inb_p(EFDR); /* Get the value from EFDR */
	return b; /* Return the value */
}

void write_sio(unsigned int index, unsigned char val_w)
{
	outb_p(index, EFER); /* Sent index at EFER */
	outb_p(val_w, EFDR); /* Send value at EFDR */
}
#endif

unsigned int read_reg(unsigned int lr, unsigned int hr)
{
	unsigned int b;
	unsigned int mod;

#if 0
	write_sio(0x07, 0x0D); /* Set Logical device number to SIORD (iLPC2AHB) */

	/* Enable SIO iLPC2AHB */
	b = read_sio(0x30);
	b |= 0x01;
	write_sio(0x30, b);
#endif

	/* Set Length to 1 Byte */
	b = sio_read(0xF8);
	b &= ~(0x03);
	sio_write(0xF8, b);

	mod = lr % 4; /* Address must be multiple of 4 */
	lr = lr - mod;

	b = hr >> 8; /* Set address */
	sio_write(0xF0, b);
	b = hr & 0x00FF;
	sio_write(0xF1, b);
	b = lr >> 8;
	sio_write(0xF2, b);
	b = lr & 0x00FF;
	sio_write(0xF3, b);

	sio_read(0xFE); /* Read Trigger */

	b = sio_read(0xF7 - mod); /* Get the value */
	return b;
}

void write_reg(unsigned char val_w, unsigned int lw, unsigned int hw)
{
	unsigned int b;
	unsigned int mod;

#if 0
	write_sio(0x07, 0x0D); /* Set Logical device number to SIORD */

	/* Enable SIO iLPC2AHB */
	b=read_sio(0x30);
	b |= 0x01;
	write_sio(0x30, b);
#endif

	/* Set Length to 1 Byte */
	b = sio_read(0xF8);
	b &= ~(0x03);
	sio_write(0xF8, b);

	b = hw >> 8; /* Set address */
	sio_write(0xF0, b);
	b = hw & 0x00FF;
	sio_write(0xF1, b);
	b = lw >> 8;
	sio_write(0xF2, b);
	b = lw & 0x00FF;
	sio_write(0xF3, b);

	sio_write(0xF7, val_w); /* Send the value */

	sio_write(0xFE, 0xCF); /* Write Trigger */
}

/*static unsigned int sio_lpc2ahb_enable(void)
{
	unsigned int b;

	b = sio_read(0x30);
	b |= 0x01;
	write_sio(0x30, b);

	return b;
}*/

void init_PECI(void)
{
	unsigned int data;

	/* select logical device iLPC2AHB */
	sio_select(SIO_LPC2AHB_LDN);
	/* enable iLPC2AHB */
	sio_logical_device_enable(SIO_LPC2AHB_EN);
	/* sio_lpc2ahb_enable(); */

	/* Enable PECI, Negotiation Timing = 0x40, Clock divider = 2 */
	write_reg(0x02, LOW_PECI_BASE_ADDR + 0x00, HIGH_PECI_BASE_ADDR);
	write_reg(0x40, LOW_PECI_BASE_ADDR + 0x01, HIGH_PECI_BASE_ADDR);
	write_reg(0x01, LOW_PECI_BASE_ADDR + 0x02, HIGH_PECI_BASE_ADDR);
	write_reg(0x00, LOW_PECI_BASE_ADDR + 0x03, HIGH_PECI_BASE_ADDR);

	/* Read length = 2, Write length = 1, CPU address = 0x30 */
	write_reg(0x30, LOW_PECI_BASE_ADDR + 0x04, HIGH_PECI_BASE_ADDR);
	write_reg(0x01, LOW_PECI_BASE_ADDR + 0x05, HIGH_PECI_BASE_ADDR);
	write_reg(0x02, LOW_PECI_BASE_ADDR + 0x06, HIGH_PECI_BASE_ADDR);
	write_reg(0x00, LOW_PECI_BASE_ADDR + 0x07, HIGH_PECI_BASE_ADDR);

	/* Write Command Code (0x01) into write Register */
	write_reg(0x01, LOW_PECI_BASE_ADDR + 0x0C, HIGH_PECI_BASE_ADDR);
	write_reg(0x00, LOW_PECI_BASE_ADDR + 0x0D, HIGH_PECI_BASE_ADDR);
	write_reg(0x00, LOW_PECI_BASE_ADDR + 0x0E, HIGH_PECI_BASE_ADDR);
	write_reg(0x00, LOW_PECI_BASE_ADDR + 0x0F, HIGH_PECI_BASE_ADDR);

	/* Fire Engine */
	write_reg(0x01, LOW_PECI_BASE_ADDR + 0x08, HIGH_PECI_BASE_ADDR);

	/* Check the status is Idle or Busy */
	data = 2;
	while (data != 0) {
		usleep(10000);
		data = read_reg(LOW_PECI_BASE_ADDR + 0x08, HIGH_PECI_BASE_ADDR);
		data &= 0x02;
	}
}

/* Read HW monitor base address */
unsigned int read_hwmon_base_address(void)
{
	unsigned int addr, high_addr, low_addr;

	if (strncmp("NCT", chip_model, 3) == 0) {
		sio_select(0x0B); /* Select Logical device B */
	} else { /* For Fintek */
		sio_select(0x04); /* Logical device number 4 */
	}

	/* Enable hardware monitor */
#if 0
	b = read_sio(0x30);
	b |= 0x01;
	write_sio(0x30, b);
#endif
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

	if (strcmp("NCT6776F", chip_model) == 0) {
		data &= ~(7); /* Clear bit0~2 */
	} else if (strcmp("NCT6779D", chip_model) == 0) {
		data &= ~(0x0F); /* Clear bit0~3 */
	}

	data |= (bank); /* Set Bank */
	outb_p(data, address + plus + 1);
}

void Type_list(unsigned int *type, float par1, float par2, float up)
{
	if (par2 == 0) {
		if (up < 20) {
			*type = 2;
		} else if (up < 110 && up >= 20) {
			*type = 0;
		} else {
			*type = 1;
		}
	} else {
		*type = 3;
	}
}

float read_sio_sensor(unsigned int address, unsigned int type, \
                      unsigned int index, unsigned int bank, \
                      float par1, float par2)
{
	int data = 0, plus;
	float result = 0;

	if (strncmp("NCT", chip_model, 3) == 0) {
		bank_select(address, bank); /* Set Bank */
	}

	if (strncmp("NCT", chip_model, 3) == 0) {
		plus = 5;
	} else if (strcmp("F71889AD", chip_model) == 0) {
		plus = 0;
	} else if (strncmp("F7186", chip_model, 5) == 0) {
		plus = 5;
	}

	if (type == 0) { /* Type=0, read Temperature */
		outb_p(index, address + plus);
		data = inb_p(address + plus + 1);

		result = data + par1;
	}

	if (type == 1) { /* Type=1, read Fan speed */
		outb_p(index, address + plus);
		data = inb_p(address + plus + 1);
		data = data << 8;
		outb_p(index + 1, address + plus);
		data |= inb_p(address + plus + 1);

		if (strncmp("F718", chip_model, 4) == 0) {
			data = 1500000 / data;
		}

		if (data == par1) /* If the fan speed = par1, set the fan speed =0 */
			data = 0;

		result = data;
	}

	if (type == 2) { /* Type=2, read Voltage. The Voltage is >0  and <2.048V */
		outb_p(index, address + plus);
		data = inb_p(address + plus + 1);
		result = (float) data * 0.008;
	}

	if (type == 3) { /* Type=3, read Voltage. The Voltage is >2.048V */
		outb_p(index, address + plus);
		data = inb_p(address + plus + 1);
		result = ((float) data) * ((float) (par1 + par2)) / ((float) par2) * 0.008;
	}

	return result;
}

float read_ast_sensor(unsigned int type, unsigned int index, \
                      float par1, float par2)
{
	unsigned int data_l = 0;
	unsigned int data_h = 0;
	unsigned int data = 0;
	unsigned int low_addr;
	unsigned int high_addr;
	int Hz = 100000;
	float result = 0;

	if (type == 0) { /* Type=0, read Temperature */
		low_addr = LOW_DATA_BASE_ADDR + index;
		high_addr = HIGH_DATA_BASE_ADDR;

		data_l = read_reg(low_addr, high_addr);
		data_h = read_reg(low_addr + 1, high_addr);
		data = data_l | (data_h << 8);
		result = data;
	}

	if (type == 1) { /* Type=1, read Temperature via I2C */
		low_addr = LOW_I2C_BASE_ADDR;
		high_addr = HIGH_I2C_BASE_ADDR;

		/* Write I2C Channel 7 clock to 100KHz */
		write_reg((Hz & 0xFF), low_addr + 0x48, high_addr);
		write_reg(((Hz >> 8) & 0xFF), low_addr + 0x49, high_addr);
		write_reg(((Hz >> 16) & 0xFF), low_addr + 0x4A, high_addr);
		write_reg(((Hz >> 24) & 0xFF), low_addr + 0x4B, high_addr);

		/* Set I2C to read, Device Address and Channel number */
		write_reg(0x07, low_addr + 0x54, high_addr);
		write_reg(index, low_addr + 0x55, high_addr);
		write_reg(0x00, low_addr + 0x56, high_addr);
		write_reg(0x00, low_addr + 0x57, high_addr);

		data = read_reg(low_addr + 0x00, high_addr); /* Enabled I2C Channel 7 */
		data |= 0x40;
		write_reg(data, low_addr + 0x00, high_addr);

		/* Fire to Engine */
		data = read_reg(low_addr + 0x5C, high_addr);
		data |= 0x01;
		write_reg(data, low_addr + 0x5C, high_addr);

		usleep(1000000); /* Need the wait time for next reading */

		data_l = 2; /* Check the status is Idle or Busy */
		while (data_l != 0) {
			usleep(10000);
			data_l = read_reg(low_addr + 0x5C, high_addr);
			data_l &= 0x02;
		}

		data =  read_reg(LOW_DATA_BASE_ADDR, HIGH_DATA_BASE_ADDR); /* Read I2C data */
		result = data;
	}

	if (type == 2) { /* Type=2, read fan */
		low_addr = LOW_TACHO_BASE_ADDR + index;
		high_addr = HIGH_TACHO_BASE_ADDR;

		data_l = read_reg(low_addr, high_addr);
		data_h = read_reg(low_addr + 1, high_addr);
		data = data_l | (data_h << 8);
		result = data;
	}

	if (type == 3) { /* Type=3, read Voltage. */
		low_addr = LOW_ADC_BASE_ADDR + index;
		high_addr = HIGH_ADC_BASE_ADDR;
		data_l = read_reg(low_addr, high_addr);
		data_h = read_reg(low_addr + 1, high_addr);
		data_h &= 0x03;
		data = data_l | (data_h << 8);
		/* Ask BIOS to get the ratio */
		result = (((float) data) * ((float) par1) * 25 / 1023 / 100);
	}

	return result;
}

float read_temperature(unsigned int address, unsigned int index, int plus, float par1, float par2)
{
	return (sio_read_reg(index, address + plus) + par1);
}

float read_fan_speed(unsigned int address, unsigned int index, int plus, float par1, float par2)
{
	int data;

	data = sio_read_reg(index, address + plus);
	data = data << 8;
	data |= sio_read_reg(index + 1, address + plus);

	if (strncmp("F718", chip_model, 4) == 0) {
		data = 1500000 / data;
	}

	if (data == par1) /* If the fan speed = par1, set the fan speed =0 */
		data = 0;

	return (data == par1) ? 0 : data;
}

float read_voltage1(unsigned int address, unsigned int index, int plus, float par1, float par2)
{
	return ((float) sio_read_reg(index, address + plus) * 0.008);
}

float read_voltage2(unsigned int address, unsigned int index, int plus, \
                    float par1, float par2)
{
	int data;
	float result;
	data = sio_read_reg(index, address + plus);
	DBG("address = %x, index = %d, par1 = %f, par2 = %f, data = %d\n", \
         address, index, par1, par2, data);
	result = ((float) data) * ((float) (par1 + par2)) / ((float) par2) * 0.008;
	
	return result;
}

float read_ast_temperature(void)
{
	float result;
	return result;
}

float read_ast_fan_speed(void)
{
	float result;
	return result;
}

float read_ast_voltage1(void)
{
	float result;
	return result;
}

float read_ast_voltage2(void)
{
	float result;
	return result;
}

/* If parameter is wrong, it will show this message */
void usage(void)
{
	printf("Usage : Command [Value]\n"
			" Value : Test time (Seconds)\n");
}
