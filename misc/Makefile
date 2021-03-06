# compiler
CC = gcc

# include directory
INCLUDE = ../include
TOOLS_DIR := ../tools

# files
LIBPCH = ../tools/libpch.c
LIBSIO = ../tools/libsio.c
TARGET = s0081_led s0951_nmi sch5027 gpioset s0391_ha_led
BIN = *.o $(TARGET)
CHANGELOG = Changelog

# objects
OBJS = ../tools/libpch.o ../tools/libsio.o
I2C_OBJS = $(TOOLS_DIR)/i2cbusses.o $(TOOLS_DIR)/util.o

GIT = $(shell which git > /dev/null 2>&1; echo $$?)

# CFLAGS
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS += -DDEBUG
else
	CFLAGS += -DNDEBUG
endif

CFLAGS += -I$(INCLUDE) -D_GNU_SOURCE

# targets
all: $(TARGET)

gpioset: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) gpioset.c -o gpioset

s0081_led: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) s0081_led.c -o s0081_led

s0951_nmi: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) s0951_nmi.c -o s0951_nmi

s0391_ha_led: $(I2C_OBJS) s0391_ha_led.c
	$(CC) $(CFLAGS) -O2 -o $@ $^

# objects
libpch.o: $(LIBPCH)
	$(CC) $(CFLAGS) -c $(LIBPCH)

libsio.o: $(LIBSIO)
	$(CC) $(CFLAGS) -c $(LIBSIO)

sch5027: libsio.o
	$(CC) $(CFLAGS) libsio.o sch5027.c -o sch5027

$(TOOLS_DIR)/i2cbusses.o: $(TOOLS_DIR)/i2cbusses.c $(INCLUDE)/i2cbusses.h $(INCLUDE)/linux/i2c-dev.h
	$(CC) $(CFLAGS) -O2 -c $< -o $@

$(TOOLS_DIR)/util.o: $(TOOLS_DIR)/util.c $(INCLUDE)/util.h
	$(CC) $(CFLAGS) -O2 -c $< -o $@

.PHONY: clean
clean:
	rm -rf $(BIN) 
