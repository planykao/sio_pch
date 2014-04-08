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
#define NCT_GPIO_IN                1
#define NCT_GPIO_OUT               0
#define NCT_GPIO0_EN_LDN           0x08
#define NCT_GPIO0_LDN              0x08
#define NCT_GPIO1_EN_LDN           0x09
#define NCT_GPIO1_LDN              0x08
#define NCT_GPIO2_EN_LDN           0x09
#define NCT_GPIO2_LDN              0x09
#define NCT_GPIO3_EN_LDN           0x09
#define NCT_GPIO3_LDN              0x09
#define NCT_GPIO4_EN_LDN           0x09
#define NCT_GPIO4_LDN              0x09
#define NCT_GPIO6_EN_LDN           0x09
#define NCT_GPIO5_LDN              0x09
#define NCT_GPIO6_EN_LDN           0x09
#define NCT_GPIO6_LDN              0x07
#define NCT_GPIO7_EN_LDN           0x09
#define NCT_GPIO7_LDN              0x07
#define NCT_GPIO8_EN_LDN           0x07
#define NCT_GPIO8_LDN              0x07
#define NCT_GPIO9_EN_LDN           0x07
#define NCT_GPIO9_LDN              0x07
#define NCT_GPIOA_EN_LDN           0x08
#define NCT_GPIOA_LDN              0x08

#define NCT_WDT_LDN                0x08
#define NCT_WDT_EN                 0
#define NCT_WDT_CTL_MODE_REG       0xF5
#define NCT_WDT_CNT_REG            0xF6
#define NCT_WDT_CTL_STA_REG        0xF7
#define NCT_WDT_CNT_MODE_OFFSET    3
#define NCT_WDT_CNT_MODE_1K_OFFSET 4
#define NCT_WDT_STATUS_OFFSET      4

#define GPIO7                      7

/* for Fintek SuperIO */
#define FINTEK_GPIO_IN         0
#define FINTEK_GPIO_OUT        1
#define FINTEK_GPIO_LDN        0x06
#define FIN_WDT_LDN            0x07
#define FIN_WDT_CONF_REG       0xF0
#define FIN_WDT_CONF_REG1      0xF5
#define FIN_WDT_CONF_REG2      0xF6
#define FIN_WDOUT_EN           (0x1 << 7)
#define FIN_WD_RST_EN          (0x1 << 0)
#define FIN_WD_UNIT_OFFSET     3
#define FIN_WD_PULSE_OFFSET    4
#define FIN_WD_EN_OFFSET       5
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

void sio_gpio_enable(int ldnum, int offset);
void sio_logical_device_enable(int bit);
int sio_gpio_get(int gpio, int index);
void sio_gpio_set(int gpio, int value, int index);
void sio_gpio_dir_in(int gpio, int index, int io);
void sio_gpio_dir_out(int gpio, int value, int index, int io);

int sio_read(int reg);
void sio_write(int reg, int val);
int sio_read_reg(int index, int address);
void sio_write_reg(int index, int address);
void sio_select(int ldnum);

/* For AST1300 */
void sio_ilpc2ahb_setup(int len);
void sio_ilpc2ahb_write(unsigned char val_w, unsigned int lw, unsigned int hw);
void sio_ilpc2ahb_writel(unsigned int val_w, unsigned int lw, unsigned int hw);
unsigned int sio_ilpc2ahb_read(int lr, int hr);
unsigned int sio_ilpc2ahb_readl(int lr, int hr);

int f71889ad_get_gpio_dir_index(int gpio);
int nct_get_gpio_dir_index(int gpio);
int sio_get_gpio_dir_index(char *chip, int gpio);

#endif
