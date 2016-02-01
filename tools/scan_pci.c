#include <stdio.h>
#include <stdlib.h>
#include <sys/io.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <unistd.h>

#define PCI_CONFIG_ADDR		0xCF8
#define PCI_CONFIG_DATA		0xCFC

#define PCI_DEVFN(dev,func)	((((dev) & 0x1f) << 3) | ((func) & 0x07))

#define PCI_CONF1_ADDRESS(bus, devfn, reg) \
	(0x80000000 | ((bus & 0xFF) << 16) | (devfn << 8) | (reg & 0xFC))

#define SBREG_BAR 0x10
#define PID_GPIOCOM1 0xAE


long int readl(long int addr);
void writel(long int val, long int addr);

long int readl(long int addr)
{
	outl(addr, PCI_CONFIG_ADDR);
	return inl(PCI_CONFIG_DATA);
}

void writel(long int val, long int addr)
{
	outl(addr, PCI_CONFIG_ADDR);
	outl(val, PCI_CONFIG_DATA);
}

unsigned int skylake_pch_readl(unsigned int *mmio, unsigned int offset);
void skylake_pch_writel(unsigned int val, unsigned int *mmio, unsigned int offset);
void skylake_pch_pair_setup(int pair, unsigned int *mmio);
void skylake_pch_cfg_setup(int cfg, unsigned int *mmio);
void skylake_pch_bp_trigger(unsigned int *mmio);
void skylake_pch_gpio_setup(unsigned int val, unsigned int *mmio);

unsigned int skylake_pch_readl(unsigned int *mmio, unsigned int offset)
{
	unsigned int volatile *tmp;
	unsigned int volatile data;
	tmp = (unsigned int *)(((void *)mmio) + offset); /* 0x540 is the offset of GPIO_D16 */
	data = *tmp;

	return data;
}

void skylake_pch_writel(unsigned int val, unsigned int *mmio, unsigned int offset)
{
	*(unsigned int volatile *)(((void *)mmio) + offset) = val;
}

/*
 * PAIR_1: GPP_G0, 0x6A8
 * PAIR_2: GPP_G1, 0x6B0
 * PAIR_3: GPP_G2, 0x6B8
 */
void skylake_pch_pair_setup(int pair, unsigned int *mmio)
{
	unsigned int data;

	data = skylake_pch_readl(mmio, 0x6A8); /* GPP_G0 */
	data &= ~((0x1 << 8) | 0x1); /* bit8:  tx buffer, bit0: tx state */
	data |= ((0x1 << 9) | (pair & 0x1)); /* bit9: rx buffer */
	skylake_pch_writel(data, mmio, 0x6A8);
	printf("0x6A8: %x\n", skylake_pch_readl(mmio, 0x6A8));

	data = skylake_pch_readl(mmio, 0x6B0); /* GPP_G1 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | ((pair >> 1) & 0x1));
	skylake_pch_writel(data, mmio, 0x6B0);
	printf("0x6B0: %x\n", skylake_pch_readl(mmio, 0x6B0));

	data = skylake_pch_readl(mmio, 0x6B8); /* GPP_G2 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | ((pair >> 2) & 0x1));
	skylake_pch_writel(data, mmio, 0x6B8);
	printf("0x6B8: %x\n", skylake_pch_readl(mmio, 0x6B8));
}

void skylake_pch_cfg_setup(int cfg, unsigned int *mmio)
{
	unsigned int data;

	data = skylake_pch_readl(mmio, 0x6C0); /* GPP_G3 */
	data &= ~((0x1 << 8) | 0x1); /* bit8:  tx buffer, bit0: tx state */
	data |= ((0x1 << 9) | (cfg & 0x1)); /* bit9: rx buffer */
	skylake_pch_writel(data, mmio, 0x6C0);
	printf("0x6C0: %x\n", skylake_pch_readl(mmio, 0x6C0));

	data = skylake_pch_readl(mmio, 0x6C8); /* GPP_G4 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | ((cfg >> 1) & 0x1));
	skylake_pch_writel(data, mmio, 0x6C8);
	printf("0x6C8: %x\n", skylake_pch_readl(mmio, 0x6C8));

	data = skylake_pch_readl(mmio, 0x6D0); /* GPP_G5 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | ((cfg >> 2) & 0x1));
	skylake_pch_writel(data, mmio, 0x6D0);
	printf("0x6D0: %x\n", skylake_pch_readl(mmio, 0x6D0));
}

void skylake_pch_bp_trigger(unsigned int *mmio)
{
	unsigned int data;

	/* low */
	data = skylake_pch_readl(mmio, 0x678); /* GPP_F18 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | 0x0);
	skylake_pch_writel(data, mmio, 0x678);
	printf("0x678: %x\n", skylake_pch_readl(mmio, 0x678));
	usleep(500000);

	/* high */
	data = skylake_pch_readl(mmio, 0x678); /* GPP_F18 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | 0x1);
	skylake_pch_writel(data, mmio, 0x678);
	printf("0x678: %x\n", skylake_pch_readl(mmio, 0x678));
	usleep(500000);

	/* low */
	data = skylake_pch_readl(mmio, 0x678); /* GPP_F18 */
	data &= ~((0x1 << 8) | 0x1);
	data |= ((0x1 << 9) | 0x0);
	skylake_pch_writel(data, mmio, 0x678);
	printf("0x678: %x\n", skylake_pch_readl(mmio, 0x678));
	usleep(500000);
}

void skylake_pch_gpio_setup(unsigned int val, unsigned int *mmio)
{
	unsigned int data, offset;
	int i;

	for (i = 0; i < 8; i++) {
		offset = 0x540 + (8 * i);
		data = skylake_pch_readl(mmio, offset); /* GPP_F18 */
		data &= ~((0x1 << 8) | 0x1);
		data |= ((0x1 << 9) | val);
		skylake_pch_writel(data, mmio, offset);
		printf("offset %x: %x\n", offset, skylake_pch_readl(mmio, offset));
	}
}

#ifdef SCAN_PCI
int main(int argc, char *argv[])
{
	int i, pair, on, off, wdt, cfg;
	unsigned int offset;
	unsigned int pch_pcr_addr, sbreg_bar;
	long int addr;
	unsigned int *mmio;
	unsigned int volatile data;
	int fd;

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

#if 0 /* haswell and below */
	for (i = 0x0; i <= 0x84; i += 0x4) {
		data = readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), i));
		printf("%02X: data = %x\n", i, data);
	}

	data = readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), 0xF0));
	printf("0xF0: data = 0x%08X\n", data);

	/* Disable GPIO Lockdown */
	writel(0x10, PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x0), 0x4C));
#else /* skylake */
	/* read argument */
#if 0
	if ((argc != 5) ||\
			(atoi(argv[1]) < 0) || (atoi(argv[1]) > 5) || \
			(atoi(argv[2]) < 0) || (atoi(argv[2]) > 1) || \
			(atoi(argv[3]) < 0) || (atoi(argv[3]) > 1) || \
			(atoi(argv[4]) < 0) || (atoi(argv[4]) > 1)) {
		printf("Usage:\n");
		printf("    # ./S1371_bypass [pair on off wdt]\n");
		printf("  pair, #        PAIR number, 0 <= PAIR <= 5\n");
		printf("  on  , #        Power on status, 0: passthrough or 1: bypass\n");
		printf("  off , #        Power off status, 0:passthrough or 1: bypass\n");
		printf("  wdt , #        WDT status, 0: reset, 1: bypass\n");
		exit(1);
	}

	pair = atoi(argv[1]);
	on = atoi(argv[2]);
	off = atoi(argv[3]);
	wdt = atoi(argv[4]);
	cfg = ((on << 2) | (off << 1) | wdt);
#endif
	if (argc != 2 || (atoi(argv[1]) < 0) || (atoi(argv[1]) > 1)) {
		printf("Usage:\n");
		printf("  # ./S1371_gpio [output]\n");
		printf("  output, #    0: pull low, 1: pull high\n");
		exit(1);
	}

	/* unhide P2SB */
	writel(0x0, PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x1), 0xE1));

	/* read base address from SBREG_BAR */
	sbreg_bar = readl(PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x1), SBREG_BAR));
	sbreg_bar &= 0xFFFFFFF0;
	printf("addrl = %x\n", sbreg_bar);

	/* hide P2SB */
	writel(0x1, PCI_CONF1_ADDRESS(0, PCI_DEVFN(0x1F, 0x1), 0xE1));

	/* calculate mmio address */
	offset = 0x540 & ~(sysconf(_SC_PAGE_SIZE) - 1); /* offset for mmap() must be page aligned */
	pch_pcr_addr = (sbreg_bar) | (PID_GPIOCOM1 << 16) | offset; /* in this case, offset is 0 */
	printf("pch addr = %x\n", pch_pcr_addr); /* pch_pcr_addr: 0xFDAE0000, fixed value */

	/* memory remap */
	fd = open("/dev/mem", O_RDWR | O_SYNC);
	mmio = mmap(0, sysconf(_SC_PAGE_SIZE), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, fd, pch_pcr_addr);
	printf("mmio: %p\n", mmio);
	if (mmio == MAP_FAILED) {
		printf("mmap failed!\n");
		exit(1);
	}

#if 0
	/* config gpio, datasheet page 1261 */
	skylake_pch_pair_setup(pair, mmio);
	skylake_pch_cfg_setup(((on << 2) | (off << 1) | wdt), mmio);
	skylake_pch_bp_trigger(mmio);
#endif
	skylake_pch_gpio_setup(atoi(argv[1]), mmio);

	munmap(mmio, sysconf(_SC_PAGE_SIZE));
	close(fd);
#endif

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

