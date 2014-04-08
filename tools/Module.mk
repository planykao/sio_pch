# objects
OBJS = $(TOOLS_DIR)/libpch.o $(TOOLS_DIR)/libsio.o
SCAN_SIO_OBJS = $(TOOLS_DIR)/libsio.o

# Tools
TOOLS_TARGET = gpio loopback hwmon bypass wdt scan_sio scan_pci

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

# Objects
$(TOOLS_DIR)/libpch.o: $(TOOLS_DIR)/libpch.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TOOLS_DIR)/libsio.o: $(TOOLS_DIR)/libsio.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(addprefix $(TOOLS_DIR)/,*.o $(TOOLS_TARGET))
