#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>

#define PCI_CONFIG_ADDR_REG		0xCF8
#define PCI_CONFIG_DATA_REG		0xCFC

#define PCI_DEVFN(dev,func)	((((dev) & 0x1f) << 3) | ((func) & 0x07))

#define PCI_CONF1_ADDRESS(bus, devfn, reg) \
	(0x80000000 | ((bus & 0xFF) << 16) | (devfn << 8) | (reg & 0xFC))

long int readl(long int addr);

long int readl(long int addr)
{
	outl(addr, PCI_CONFIG_ADDR_REG);
	return inl(PCI_CONFIG_DATA_REG);
}

int main(void)
{
	int i;
	long int data;

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	for (i = 0x0; i <= 0x84; i += 0x4) {
		data = readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), i));
		printf("%02X: data = %x\n", i, data);
	}

	return 0;
}

