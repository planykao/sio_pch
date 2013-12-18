#ifndef _MSI_SIOLIB_H
#define _MSI_SIOLIB_H

/* GPIO register address from SuperIO */
#define EFER                0x4E
#define EFDR                0x4F
#define SIO_LDSEL_REG       0x07
#define SIO_ENABLE_REG      0x30
#define SIO_GPIO_EN_REG     0x09
#define SIO_GPIO7_DIR_REG   0xE0
#define SIO_GPIO7_DATA_REG  0xE1
#define SIO_GPIO7_EN_OFFSET (0x1 << 7)
#define SIO_GPIO7_LDN       0x07

void sio_gpio_enable(int ldnum);
unsigned char sio_gpio_get(int gpio);
void sio_gpio_set(int gpio, int value);
void sio_gpio_dir_in(int gpio);
void sio_gpio_dir_out(int gpio, int value);
void sio_enter(void);
void sio_exit(void);
unsigned char sio_read(int reg);
void sio_write(int reg, unsigned char val);
void sio_select(int ldnum);

#endif
