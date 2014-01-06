CC = gcc
CONFDIR = ${PWD}/conf
SCRIPTDIR = ${PWD}/script
RELEASEDIR = ${PWD}/release
INCLUDE = ${PWD}/
LP_RELEASE_DIR = $(RELEASEDIR)/loopback
BP_RELEASE_DIR = $(RELEASEDIR)/bypass
HWMON_RELEASE_DIR = $(RELEASEDIR)/hwmon
GPIO_OBJS = pchlib.o siolib.o
HWMON_OBJS = siolib.o

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

.PHONY: release	
release:
	cp -rf $(CONFDIR)/* $(RELEASEDIR)
	cp -rf $(SCRIPTDIR)/* $(RELEASEDIR)
	if [ -f gpio-loopback ]; then cp -rf gpio-loopback $(LP_RELEASE_DIR); fi
	if [ -f bypass ]; then cp -rf bypass $(BP_RELEASE_DIR); fi
	if [ -f hwmon ]; then cp -rf hwmon $(HWMON_RELEASE_DIR); fi

.PHONY: clean
clean:
	rm -rf *.o gpio-loopback hwmon $(RELEASEDIR)/*
