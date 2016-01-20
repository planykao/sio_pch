#ifndef _MSI_PCH_H
#define _MSI_PCH_H

/* GPIO register address from PCH */
#define GPIO_USE_SEL1 0x00 /* GPIO_USE_SEL1 offset */
#define GPIO_USE_SEL2 0x30 /* GPIO_USE_SEL2 offset */
#define GPIO_USE_SEL3 0x40 /* GPIO_USE_SEL3 offset */

#define GP_IO_SEL1 0x04 /* GPIO Input/Output Select1 offset */
#define GP_IO_SEL2 0x34 /* GPIO Input/Output Select2 offset */
#define GP_IO_SEL3 0x44 /* GPIO Input/Output Select3 offset */

#define GP_LVL1 0x0C /* GPIO Level1 for Input or Output offset */
#define GP_LVL2 0x38 /* GPIO Level2 for Input or Output offset */
#define GP_LVL3 0x48 /* GPIO Level3 for Input or Output offset */

#define GPO_BLINK 0x18 /* GPIO Blink Enable Register */

#define GPIO_USE_SEL1_ADDR(addr) (addr + GPIO_USE_SEL1) 
#define GPIO_USE_SEL2_ADDR(addr) (addr + GPIO_USE_SEL2)
#define GPIO_USE_SEL3_ADDR(addr) (addr + GPIO_USE_SEL3)

#define GP_IO_SEL1_ADDR(addr) (addr + GP_IO_SEL1)
#define GP_IO_SEL2_ADDR(addr) (addr + GP_IO_SEL2)
#define GP_IO_SEL3_ADDR(addr) (addr + GP_IO_SEL3)

#define GP_LVL1_ADDR(addr) (addr + GP_LVL1)
#define GP_LVL2_ADDR(addr) (addr + GP_LVL2)
#define GP_LVL3_ADDR(addr) (addr + GP_LVL3)

int gpio_setup_addr(unsigned int *gpio_use_sel_addr, \
                    unsigned int *gp_io_sel_addr, \
                    unsigned int *gp_lvl_addr, \
                    int gpio, unsigned int base_addr);
void gpio_enable(unsigned int gpio_use_sel_addr, int gpio);
unsigned long int gpio_get(unsigned int gpio_lvl_addr, int gpio);
void gpio_set(unsigned int gpio_lvl_addr, int gpio, int value);
void gpio_dir_in(unsigned int gp_io_sel_addr, int gpio);
void gpio_dir_out(unsigned int gp_io_sel_addr, \
                  unsigned int gp_lvl_addr, int gpio, int value);

void gpio_blink(unsigned int base, int gpio, int value);

#endif
