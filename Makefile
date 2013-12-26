CC = gcc
CONFDIR = ${PWD}/conf
RELEASEDIR = ${PWD}/release
INCLUDE = ${PWD}/
LP_RELEASE_DIR = $(RELEASEDIR)/loopback
BP_RELEASE_DIR = $(RELEASEDIR)/bypass
HWMON_RELEASE_DIR = $(RELEASEDIR)/hwmon
GPIO_OBJS = pchlib.o siolib.o
HWMON_OBJS = siolib.o pin_list.o

DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS = -DDEBUG
else
	CFLAGS = -DNDEBUG
endif

all: gpio hwmon release

gpio: $(GPIO_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(GPIO_OBJS) gpio-loopback.c \
		-o gpio-loopback

hwmon: $(HWMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(HWMON_OBJS) hwmon.c -o hwmon

pchlib.o: pchlib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c pchlib.c

siolib.o: siolib.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c siolib.c

pin_list.o: pin_list.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c pin_list.c

.PHONY: release	
release:
	mkdir -p $(LP_RELEASE_DIR)
	mkdir -p $(BP_RELEASE_DIR)
	mkdir -p $(HWMON_RELEASE_DIR)
	cp -rf gpio-loopback $(LP_RELEASE_DIR)
	cp -rf bypass $(BP_RELEASE_DIR)
	cp -rf hwmon $(HWMON_RELEASE_DIR)
	cp -rf $(CONFDIR)/loopback/* $(LP_RELEASE_DIR)
	cp -rf $(CONFDIR)/bypass/* $(BP_RELEASE_DIR)
	cp -rf $(CONFDIR)/hwmon/* $(HWMON_RELEASE_DIR)

.PHONY: clean
clean:
	rm -rf *.o gpio-loopback hwmon $(RELEASEDIR)/*
