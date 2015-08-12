#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>

#define PCI_CONFIG_ADDR		0xCF8
#define PCI_CONFIG_DATA		0xCFC

#define PCI_DEVFN(dev,func)	((((dev) & 0x1f) << 3) | ((func) & 0x07))

#define PCI_CONF1_ADDRESS(bus, devfn, reg) \
	(0x80000000 | ((bus & 0xFF) << 16) | (devfn << 8) | (reg & 0xFC))

long int readl(long int addr);
long int writel(long int val, long int addr);

long int readl(long int addr)
{
	outl(addr, PCI_CONFIG_ADDR);
	return inl(PCI_CONFIG_DATA);
}

long int writel(long int val, long int addr)
{
	outl(addr, PCI_CONFIG_ADDR);
	outl(val, PCI_CONFIG_DATA);
}

#ifdef SCAN_PCI
int main(void)
{
	int i;
	unsigned int data;

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	for (i = 0x0; i <= 0x84; i += 0x4) {
		data = readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), i));
		printf("%02X: data = %x\n", i, data);
	}

	data = readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), 0xF0));
	printf("0xF0: data = 0x%08X\n", data);

	/* Disable GPIO Lockdown */
	writel(0x10, PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), 0x4C));

	return 0;
}
#else
unsigned int scan_pch_gpio_base(void)
{
	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	return (readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), 0x48)) - 1);
}
#endif

