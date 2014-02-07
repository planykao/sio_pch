#ifndef _MSI_SIOLIB_H
#define _MSI_SIOLIB_H

/* Register address from SuperIO */
#define SIO_LDSEL_REG              0x07
#define SIO_ENABLE_REG             0x30
#define SIO_HW_BASE_REG            0x60
#define SIO_GPIO_EN_REG            0x09
#define SIO_GPIO7_DIR_REG          0xE0
#define SIO_GPIO7_DATA_REG         0xE1
#define SIO_GPIO7_ENABLE           (0x1 << 7)
#define SIO_GPIO7_LDN              0x07

/* for Nuvoton SuperIO */
#define NCT_WDT_LDN                0x08
#define NCT_WDT_EN                 0
#define NCT_WDT_CTL_MODE_REG       0xF5
#define NCT_WDT_CNT_REG            0xF6
#define NCT_WDT_CTL_STA_REG        0xF7
#define NCT_WDT_CNT_MODE_OFFSET    3
#define NCT_WDT_CNT_MODE_1K_OFFSET 4
#define NCT_WDT_STATUS_OFFSET      4

/* for Fintek SuperIO */
#define FINTEK_GPIO_LDN        0x06
#define FIN_WDT_LDN            0x07
#define FIN_WDT_CONF_REG       0xF0
#define FIN_WDT_CONF_REG1      0xF5
#define FIN_WDT_CONF_REG2      0xF6
#define FIN_WDOUT_EN           (0x1 << 7)
#define FIN_WD_RST_EN          (0x1 << 0)
#define FIN_WD_EN              (0x1 << 5)
#define FIN_WD_UNIT_OFFSET     3
#define FIN_WDTMOUT_STS_OFFSET 6

/* For Aspeed */
#define SIO_HWMON_EN         0
#define SIO_LPC2AHB_LDN      0x0D
#define SIO_LPC2AHB_EN       0

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

unsigned int EFER;
unsigned int EFDR;
unsigned int SIO_ADDR_REG_OFFSET; /* Address Port register offset */
unsigned int SIO_DATA_REG_OFFSET; /* Data Port register offset */

void sio_enter(char *chip);
void sio_exit(void);

void sio_gpio_enable(int ldnum);
void sio_logical_device_enable(int bit);
int sio_gpio_get(int gpio);
void sio_gpio_set(int gpio, int value);
void sio_gpio_dir_in(int gpio);
void sio_gpio_dir_out(int gpio, int value);

int sio_read(int reg);
void sio_write(int reg, int val);
int sio_read_reg(int index, int address);
void sio_write_reg(int index, int address);
void sio_select(int ldnum);

/* For AST1300 */
void sio_ilpc2ahb_setup(void);
void sio_ilpc2ahb_write(unsigned char val_w, unsigned int lw, unsigned int hw);
unsigned int sio_ilpc2ahb_read(int lr, int hr);
#endif
