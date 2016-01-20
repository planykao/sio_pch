#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/io.h>
#include <errno.h>

#ifdef DEBUG
#include <time.h>
#include <sys/time.h>
#endif

#include <sitest.h>
#include <libsio.h>

#define AST_SCU_BASE_HIGH_ADDR        0x1E6E
#define AST_SCU_BASE_LOW_ADDR         0x2000
#define AST_SCU_MULTI_FNC_PIN_ADDR    AST_SCU_BASE_LOW_ADDR + 0x84
#define AST_SCU_UNLOCK                0x1688A8A8
#define AST_WDT_BASE_HIGH_ADDR        0x1E78
#define AST_WDT_BASE_LOW_ADDR         0x5000

#define AST_WDT1_STATUS_ADDR          AST_WDT_BASE_LOW_ADDR + 0x00
#define AST_WDT1_RELOAD_ADDR          AST_WDT_BASE_LOW_ADDR + 0x04
#define AST_WDT1_RESTART_ADDR         AST_WDT_BASE_LOW_ADDR + 0x08
#define AST_WDT1_CTL_ADDR             AST_WDT_BASE_LOW_ADDR + 0x0C
#define AST_WDT1_TOUT_STATUS_ADDR     AST_WDT_BASE_LOW_ADDR + 0x10
#define AST_WDT1_EVENT_CNT_ADDR       AST_WDT_BASE_LOW_ADDR + 0x11
#define AST_WDT1_CLR_TOUT_STATUS_ADDR AST_WDT_BASE_LOW_ADDR + 0x14
#define AST_WDT1_RST_WIDTH_ADDR       AST_WDT_BASE_LOW_ADDR + 0x18

#define AST_WDT2_STATUS_ADDR          AST_WDT_BASE_LOW_ADDR + 0x20
#define AST_WDT2_RELOAD_ADDR          AST_WDT_BASE_LOW_ADDR + 0x24
#define AST_WDT2_RESTART_ADDR         AST_WDT_BASE_LOW_ADDR + 0x28
#define AST_WDT2_CTL_ADDR             AST_WDT_BASE_LOW_ADDR + 0x2C
#define AST_WDT2_TOUT_STATUS_ADDR     AST_WDT_BASE_LOW_ADDR + 0x30
#define AST_WDT2_EVENT_CNT_ADDR       AST_WDT_BASE_LOW_ADDR + 0x31
#define AST_WDT2_CLR_TOUT_STATUS_ADDR AST_WDT_BASE_LOW_ADDR + 0x34
#define AST_WDT2_RST_WIDTH_ADDR       AST_WDT_BASE_LOW_ADDR + 0x38

#define AST_WDT_STATUS_ADDR(x)          AST_WDT##x##_STATUS_ADDR
#define AST_WDT_RELOAD_ADDR(x)          AST_WDT##x##_RELOAD_ADDR 
#define AST_WDT_RESTART_ADDR(x)         AST_WDT##x##_RESTART_ADDR
#define AST_WDT_CTL_ADDR(x)             AST_WDT##x##_CTL_ADDR
#define AST_WDT_TOUT_STATUS_ADDR(x)     AST_WDT##x##_TOUT_STATUS_ADDR
#define AST_WDT_EVENT_CNT_ADDR(x)       AST_WDT##x##_EVENT_CNT_ADDR
#define AST_WDT_CLR_TOUT_STATUS_ADDR(x) AST_WDT##x##_CLR_TOUT_STATUS_ADDR
#define AST_WDT_RST_WIDTH_ADDR(x)       AST_WDT##x##_RST_WIDTH_ADDR

#define CLOCK                         1000000 /* 1MHz */

void usage(int);
void nct_wdt_setup(int time);
void fin_wdt_setup(int time);
void ast_wdt_setup(int time);

#ifdef DEBUG
void monitor(void);
void read_counter_status(void);
#endif

extern void initcheck(void);

struct timeval start;

int main(int argc, char *argv[])
{
	FILE *fp;
	char chip[10], args, *filename;
	unsigned int base_addr;
	int i, time = -1;

	initcheck();

	if (iopl(3)) {
		perror(NULL);
		exit(1);
	}

	/* parsing arguments */
	while ((args = getopt(argc, argv, "c:t:h")) != -1) {
		switch (args) {
			case 'c':
				filename = optarg;
				break;
			case 't':
				time = atoi(optarg);
				break;
			case ':':
			case 'h':
			case '?':
				usage(1);
				break;
		}
	}

	if (filename == NULL || time == -1)
		usage(0);

	/* read configuration file */
	fp = fopen(filename, "r");
	if (fp == NULL) {
		ERR("Fail to open <%s>, please enter the correct file name.\n", argv[1]);
		usage(1);
	}

	/* Skip the header */
	for (i = 0; i < 5; i++)
		fscanf(fp, "%*[^\n]\n", NULL);

	fscanf(fp, "%[^,], %x\n", chip, &base_addr);
	DBG("chip: %s, base_addr = %x\n", chip, base_addr);

	fclose(fp);

	EFER = base_addr;
	EFDR = base_addr + 1;

	sio_enter(chip);

	/* setup the watchdog */
	if (strncmp(chip, "NCT", 3) == 0)
		nct_wdt_setup(time);
	else if (strncmp(chip, "F71", 3) == 0)
		fin_wdt_setup(time);
	else if (strncmp(chip, "AST", 3) == 0)
		ast_wdt_setup(time);
	else
		ERR("this program doesn't support <%s> yet.\n", chip);

	sio_exit();

	return 0;
}

void nct_wdt_setup(int time)
{
	int b;

	/* Select Logical Device Number for WatchDogTimer */
	sio_select(NCT_WDT_LDN);

	/* Enable Watchdog */
	sio_logical_device_enable(NCT_WDT_EN);

	/* Set Watchdog Timer I count mode to 'second' */
	b = sio_read(NCT_WDT_CTL_MODE_REG);
	b &= ~((0x1 << NCT_WDT_CNT_MODE_1K_OFFSET) | \
			(0x1 << NCT_WDT_CNT_MODE_OFFSET));
	sio_write(NCT_WDT_CTL_MODE_REG, b);

	/* Set Watchdog timer value */
	sio_write(NCT_WDT_CNT_REG, time);

	/* Clear Watchdog status */
	b = sio_read(NCT_WDT_CTL_STA_REG);
	b &= ~(0x1 << NCT_WDT_STATUS_OFFSET);
	sio_write(NCT_WDT_CTL_STA_REG, b);
}

/*
 * fin_wdt_setup(), setup the WatchDog Timer.
 * TODO: WDTRST is multi-function pin, when BIOS disable watchdog, this bit
 *       should be 0(GPIO14), and the watchdog reset will fail. Need to test
 *       and verify.
 */
void fin_wdt_setup(int time)
{
	int b;

	/* Select Logical Device Number for WatchDogTimer */
	sio_select(FIN_WDT_LDN);

	/* Enable Watchdog */
	sio_logical_device_enable(FIN_WDT_EN_OFFSET);
	b = sio_read(FIN_WDT_CONF_REG);
	b &= ~(FIN_WDOUT_EN | FIN_WD_RST_EN);
	b |= (FIN_WDOUT_EN | FIN_WD_RST_EN);
	DBG("write 0x%X to FIN_WDT_CONF_REG(0x%X)\n", b, FIN_WDT_CONF_REG);
	sio_write(FIN_WDT_CONF_REG, b);

	/* Set time of watchdog timer */
	sio_write(FIN_WDT_CONF_REG2, time);
	DBG("FIN_WDT_CONF_REG2(0x%X) = 0x%X\n", FIN_WDT_CONF_REG2, sio_read(FIN_WDT_CONF_REG2));

	/* 
	 * Select time unit(0: second, 1: 60 seconds), clear status and start 
	 * counting.
	 */
	b = sio_read(FIN_WDT_CONF_REG1);
	DBG("read from FIN_WDT_CONF_REG1(0x%X) = 0x%X\n", FIN_WDT_CONF_REG1, b);
#if 1
	b &= ~((0x1 << FIN_WD_UNIT_OFFSET) | (0x3 << FIN_WD_PSWIDTH_OFFSET));
	b |= ((0x1 << FIN_WD_PULSE_OFFSET) | (0x1 << FIN_WD_EN_OFFSET) | \
			(0x1 << FIN_WDTMOUT_STS_OFFSET) | (0x1 << FIN_WD_PSWIDTH_OFFSET));
#else
	b &= ~((0x1 << FIN_WD_UNIT_OFFSET) | (0x1 << FIN_WD_PULSE_OFFSET));
	b |= ((0x1 << FIN_WD_EN_OFFSET) | \
			(0x1 << FIN_WDTMOUT_STS_OFFSET));
#endif
	DBG("write 0x%X to FIN_WDT_CONF_REG1(0x%X)\n", b, FIN_WDT_CONF_REG1);
	sio_write(FIN_WDT_CONF_REG1, b);
}

void ast_wdt_setup(int time)
{
	unsigned int data, reload = CLOCK * time;

	/*
	 * Disable WDT and select 1MHz as source clock
	 * WARNING: The source clock can't configure as PCLK, 
	 */
	DBG("0x0C: %x\n", sio_ilpc2ahb_readl(AST_WDT1_CTL_ADDR, AST_WDT_BASE_HIGH_ADDR));
	data = sio_ilpc2ahb_readl(AST_WDT1_CTL_ADDR, AST_WDT_BASE_HIGH_ADDR);
#ifdef DEBUG
	data &= ~((0x1 << 0) | (0x1 << 2) | (0x1 << 3));
#else
	data &= ~(0x1 << 0);
	data |= (0x1 << 4); /* Make sure the clock is 1MHz */
#endif
	sio_ilpc2ahb_writel(data, AST_WDT1_CTL_ADDR, AST_WDT_BASE_HIGH_ADDR);
	DBG("0x0C: %x\n", sio_ilpc2ahb_readl(AST_WDT1_CTL_ADDR, AST_WDT_BASE_HIGH_ADDR));

	/* 
	 * Write 0x3B value into WDT1 Clear Timeout Status Register to clear 
	 * WDT counter register.
	 */
	sio_ilpc2ahb_writel(0x3B, AST_WDT1_CLR_TOUT_STATUS_ADDR, \
			AST_WDT_BASE_HIGH_ADDR);

	/* Set WDT1 Counter Reload Value */
	sio_ilpc2ahb_writel(reload, AST_WDT1_RELOAD_ADDR, AST_WDT_BASE_HIGH_ADDR);

	/*
	 * Write 0x4755 into WDT1 Counter Restart Register to reload the counter, 
	 * this action will load AST_WDT1_RELOAD_ADDR into AST_WDT1_STATUS_ADDR
	 */
	sio_ilpc2ahb_writel(0x4755, AST_WDT1_RESTART_ADDR, AST_WDT_BASE_HIGH_ADDR);

	/*
	 * Unclock SCU register 
	 * Write 0x1688A8A8 to unlock this register.
	 * Write other value to lock this register.
	 */
	sio_ilpc2ahb_writel(AST_SCU_UNLOCK, AST_SCU_BASE_LOW_ADDR, AST_SCU_BASE_HIGH_ADDR);


	/* Enable WDT output function pin in SCU[84] D[4]/D[5] */
	DBG("SCU: %x\n", sio_ilpc2ahb_readl(AST_SCU_MULTI_FNC_PIN_ADDR, AST_SCU_BASE_HIGH_ADDR));
	data = sio_ilpc2ahb_readl(AST_SCU_MULTI_FNC_PIN_ADDR, AST_SCU_BASE_HIGH_ADDR);
	data |= (0x3 << 4);
	sio_ilpc2ahb_writel(data, AST_SCU_MULTI_FNC_PIN_ADDR, AST_SCU_BASE_HIGH_ADDR);
	DBG("SCU: %x\n", sio_ilpc2ahb_readl(AST_SCU_MULTI_FNC_PIN_ADDR, AST_SCU_BASE_HIGH_ADDR));

	/* Enable Watchdog timer */
	data = sio_ilpc2ahb_readl(AST_WDT1_CTL_ADDR, AST_WDT_BASE_HIGH_ADDR);
	DBG("data = %x\n", data);
#ifdef DEBUG
	data |= (0x1 << 0 | (0x1 << 4));
#else
	data |= ((0x1 << 0) | (0x1 << 2) | (0x1 << 3) | (0x1 << 4));
#endif
	DBG("data = %x\n", data);
	sio_ilpc2ahb_writel(data, AST_WDT1_CTL_ADDR, AST_WDT_BASE_HIGH_ADDR);

#ifdef DEBUG
	monitor();
#endif
}

#ifdef DEBUG
/*
 * When the counter is descreased to 0 and WDT0C[4](wdt_intr) is enable, 
 * the WDT will trigger an interrupt and AST_WDT1_EVENT_CNT_ADDR will be 
 * increase. Monitor AST_WDT1_EVENT_CNT_ADDR and calculate the time to verify
 * the precision of WDT.
 */
void monitor()
{
	unsigned long int pre, cur;
	double pre_time, cur_time;

	cur = sio_ilpc2ahb_read(AST_WDT1_EVENT_CNT_ADDR, AST_WDT_BASE_HIGH_ADDR);
	gettimeofday(&start, NULL);
	pre_time = cur_time = ((double)(start.tv_sec * 1000000) + \
			(double)start.tv_usec) / 1000000;

	while (1) {
		pre = cur;
		cur = sio_ilpc2ahb_read(AST_WDT1_EVENT_CNT_ADDR, AST_WDT_BASE_HIGH_ADDR);

		if (pre != cur) {
			gettimeofday(&start, NULL);
			cur_time = ((double)(start.tv_sec * 1000000) + \
					(double)start.tv_usec) / 1000000;
			printf("time: %f\n", cur_time - pre_time);
			pre_time = cur_time;
			break;
		}
	}
}

/* Read Counter Status */
void read_counter_status()
{
	unsigned int data;

	data = sio_ilpc2ahb_read(AST_WDT1_STATUS_ADDR, AST_WDT_BASE_HIGH_ADDR);
	data |= (sio_ilpc2ahb_read(AST_WDT1_STATUS_ADDR + 1, \
				AST_WDT_BASE_HIGH_ADDR) << 8);
	data |= (sio_ilpc2ahb_read(AST_WDT1_STATUS_ADDR + 2, \
				AST_WDT_BASE_HIGH_ADDR) << 16);
	data |= (sio_ilpc2ahb_read(AST_WDT1_STATUS_ADDR + 3, \
				AST_WDT_BASE_HIGH_ADDR) << 24);
	printf("Counter Status = %x\n", data);
}
#endif

void usage(int i)
{
	if (i) {
		printf("Usage : ./wdt [-c config -t time]\n");
		printf("Usage : ./wdt [-h]\n\n");
		printf("  -c, <config_file>  configuration file for target board\n");
		printf("  -t, #              time(seconds) of watcdog timer\n");
	} else {
		printf("Usage: [-c config|-t time|-h]\n");
		printf("Try './wdt -h for more information\n'");
	}

	exit(1);
}
