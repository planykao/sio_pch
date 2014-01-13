# compiler
CC = gcc

# include directory
INCLUDE = ${PWD}/

# directory
CONFDIR = ${PWD}/conf
SCRIPTDIR = ${PWD}/script
RELEASEDIR = ${PWD}/release
LP_RELEASE_DIR = $(RELEASEDIR)/loopback
BP_RELEASE_DIR = $(RELEASEDIR)/bypass
HWMON_RELEASE_DIR = $(RELEASEDIR)/hwmon

# objects
GPIO_OBJS = libpch.o libsio.o
HWMON_OBJS = libsio.o
BP_OBJS = libsio.o

BIN = bypass gpio-loopback hwmon
GIT := $(shell which git 2> /dev/null)
CHANGELOG = Changelog

# debug flag
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CFLAGS = -DDEBUG
else
	CFLAGS = -DNDEBUG
endif

all: gpio hwmon bypass changelog release

gpio: $(GPIO_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(GPIO_OBJS) gpio-loopback.c \
		-o gpio-loopback

hwmon: $(HWMON_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(HWMON_OBJS) hwmon.c -o hwmon

bypass: $(BP_OBJS)
	$(CC) $(CFLAGS) -I$(INCLUDE) $(BP_OBJS) bypass.c -o bypass

libpch.o: libpch.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c libpch.c

libsio.o: libsio.c
	$(CC) $(CFLAGS) -I$(INCLUDE) -c libsio.c

changelog:
ifndef GIT
	@echo "git not found, please install git first."
else
	@git log > $(CHANGELOG)
endif

.PHONY: release	
release:
	cp -rf $(CONFDIR)/* $(RELEASEDIR)
	cp -rf $(SCRIPTDIR)/* $(RELEASEDIR)
	if [ -f gpio-loopback ]; then cp -rf gpio-loopback $(LP_RELEASE_DIR); fi
	if [ -f bypass ]; then cp -rf bypass $(BP_RELEASE_DIR); fi
	if [ -f hwmon ]; then cp -rf hwmon $(HWMON_RELEASE_DIR); fi

.PHONY: clean
clean:
	rm -rf *.o $(BIN) $(RELEASEDIR)/* $(CHANGELOG)
