# objects
OBJS = $(TOOLS_DIR)/libpch.o $(TOOLS_DIR)/libsio.o $(TOOLS_DIR)/useless_crypto.o
SCAN_SIO_OBJS = $(TOOLS_DIR)/libsio.o
I2C_OBJS = $(TOOLS_DIR)/i2cbusses.o $(TOOLS_DIR)/util.o $(TOOLS_DIR)/scan_pci.o $(TOOLS_DIR)/libpch.o

# Tools
TOOLS_TARGET = gpio loopback hwmon bypass wdt scan_sio i2cget nct6683d read_fw_ver

all: $(addprefix $(TOOLS_DIR)/,$(TOOLS_TARGET))

$(TOOLS_DIR)/gpio: $(OBJS) $(TOOLS_DIR)/gpio.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/loopback: $(OBJS) $(TOOLS_DIR)/loopback.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/hwmon: $(OBJS) $(TOOLS_DIR)/hwmon.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/bypass: $(OBJS) $(TOOLS_DIR)/bypass.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/wdt: $(OBJS) $(TOOLS_DIR)/wdt.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/scan_sio: $(SCAN_SIO_OBJS) $(TOOLS_DIR)/scan_sio.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/scan_pci: $(TOOLS_DIR)/scan_pci.c
	$(CC) $(CFLAGS) -I/usr/src/kernels/2.6.32-358.el6.x86_64/include/ -o $@ $^

$(TOOLS_DIR)/i2cget: $(I2C_OBJS) $(TOOLS_DIR)/i2cget.c
	$(CC) $(CFLAGS) -O2 -lcurses -o $@ $^

$(TOOLS_DIR)/nct6683d: $(OBJS) $(TOOLS_DIR)/nct6683d.c
	$(CC) $(CFLAGS) -o $@ $^

$(TOOLS_DIR)/read_fw_ver: $(OBJS) $(TOOLS_DIR)/read_fw_ver.c
	$(CC) $(CFLAGS) -o $@ $^

# Objects
$(TOOLS_DIR)/libpch.o: $(TOOLS_DIR)/libpch.c $(INCLUDE)/libpch.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TOOLS_DIR)/libsio.o: $(TOOLS_DIR)/libsio.c $(INCLUDE)/libsio.h
	$(CC) $(CFLAGS) -c $< -o $@

$(TOOLS_DIR)/useless_crypto.o: $(TOOLS_DIR)/useless_crypto.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TOOLS_DIR)/i2cbusses.o: $(TOOLS_DIR)/i2cbusses.c $(INCLUDE)/i2cbusses.h $(INCLUDE)/linux/i2c-dev.h
	$(CC) $(CFLAGS) -O2 -c $< -o $@

$(TOOLS_DIR)/util.o: $(TOOLS_DIR)/util.c $(INCLUDE)/util.h
	$(CC) $(CFLAGS) -O2 -c $< -o $@

$(TOOLS_DIR)/scan_pci.o: $(TOOLS_DIR)/scan_pci.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(addprefix $(TOOLS_DIR)/,*.o $(TOOLS_TARGET))
